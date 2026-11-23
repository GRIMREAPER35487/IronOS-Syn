
#include "OperatingModes.h"
#include "SolderingCommon.h"
#include "ui_drawing.hpp"
// State 1 = button locking  (0:unlocked+released, 1:unlocked, 2:locked, 3:locked+released)
// State 2 = boost mode
// State 3 = buzzer timer
// State 4 = set after the first frame in this mode (custom)
// State 5 = set point reached this heat-up, so the reached signal has already fired
// State 6 = at-temp screen blink running (custom)
// State 8 = tick the at-temp screen blink started (custom)

// Screen blink the first time the tip reaches the set point: inverted on phases 0, 2, 4
#define AT_TEMP_BLINK_PHASE  (TICKS_100MS * 2)
#define AT_TEMP_BLINK_PHASES 6

// Heat-up countdown for the basic soldering screen (custom).
// The heating rate is measured over short windows and smoothed; the countdown is how long the rest of the climb to
// within 10 C of the set point takes at that rate. It is only shown while clearly heating, otherwise -1.
#define HEAT_ETA_WINDOW    (TICKS_100MS * 5) // rate sample length; long enough that whole-degree steps don't dominate
#define HEAT_ETA_STALE     TICKS_SECOND      // longer than this since the last call: start measuring again
#define HEAT_ETA_SHOW_RATE 30                // x10 C/s: start showing the countdown above 3 C/s ...
#define HEAT_ETA_HIDE_RATE 15                // ... and keep showing it until the rate drops below 1.5 C/s (no flicker)
#define HEAT_ETA_MAX       99                // seconds; two digits on screen

static int8_t heatUpEtaSeconds(int errorC, bool atTemp) {
  static TickType_t lastTick   = 0;
  static TickType_t lastCall   = 0;
  static int32_t    lastTempC  = 0;
  static int32_t    rateX10    = 0; // smoothed heating rate, tenths of a C per second
  static bool       showing    = false;
  const TickType_t  now        = xTaskGetTickCount();
  const int32_t     tipC       = TipThermoModel::getTipInC();
  const bool        restarting = (now - lastCall) > HEAT_ETA_STALE;
  lastCall                     = now;
  if (restarting) {
    lastTick  = now;
    lastTempC = tipC;
    rateX10   = 0;
    showing   = false;
    return -1;
  }
  if ((now - lastTick) >= HEAT_ETA_WINDOW) {
    const int32_t sampleX10 = ((tipC - lastTempC) * 10 * (int32_t)TICKS_SECOND) / (int32_t)(now - lastTick);
    rateX10                 = (rateX10 * 6 + sampleX10 * 4) / 10;
    lastTick                = now;
    lastTempC               = tipC;
  }
  showing = !atTemp && errorC > 10 && rateX10 >= (showing ? HEAT_ETA_HIDE_RATE : HEAT_ETA_SHOW_RATE);
  if (!showing) {
    return -1;
  }
  int32_t eta = ((errorC - 10) * 10 + rateX10 - 1) / rateX10; // round up
  if (eta > HEAT_ETA_MAX) {
    eta = HEAT_ETA_MAX;
  }
  return (int8_t)eta;
}

OperatingMode handleSolderingButtons(const ButtonState buttons, guiContext *cxt) {
  if (cxt->scratch_state.state1 >= 2) {
    // Buttons are currently locked
    if (buttons == BUTTON_BOTH_LONG) {
      if (cxt->scratch_state.state1 == 3) {
        // Unlocking
        if (warnUser(translatedString(Tr->UnlockingKeysString), buttons)) {
          cxt->scratch_state.state1 = 1;
          cxt->scratch_state.state7 = 0;
        }
      } else {
        warnUser(translatedString(Tr->LockingKeysString), buttons);
      }
      return OperatingMode::Soldering;
    }
    if (cxt->scratch_state.state7 != 0) {
      // show locked until timer is up
      if (xTaskGetTickCount() >= cxt->scratch_state.state7) {
        cxt->scratch_state.state7 = 0;
      } else {
        warnUser(translatedString(Tr->WarningKeysLockedString), buttons);
        return OperatingMode::Soldering;
      }
    }
    switch (buttons) {
    case BUTTON_NONE:
      cxt->scratch_state.state1 = 3;
      cxt->scratch_state.state2 = 0;
      break;
    case BUTTON_F_LONG:
      if (getSettingValue(SettingsOptions::BoostTemp) && (getSettingValue(SettingsOptions::LockingMode) == lockingMode_t::BOOST)) {
        cxt->scratch_state.state2 = 1;
        break;
      }
    /*Fall through*/
    default: // Set timer for and display a lock warning
      cxt->scratch_state.state7 = xTaskGetTickCount() + TICKS_SECOND;
      warnUser(translatedString(Tr->WarningKeysLockedString), buttons);
      break;
    }
    return OperatingMode::Soldering;
  }

  bool detailedView = getSettingValue(SettingsOptions::DetailedIDLE) && getSettingValue(SettingsOptions::DetailedSoldering);
  // otherwise we are unlocked
  switch (buttons) {
  case BUTTON_NONE:
    cxt->scratch_state.state2 = 0;
    cxt->scratch_state.state1 = 0;
    break;
  case BUTTON_BOTH:
  /*Fall through*/
  case BUTTON_B_LONG:
    cxt->transitionMode = detailedView ? TransitionAnimation::None : TransitionAnimation::Right;
    return OperatingMode::HomeScreen;
  case BUTTON_F_LONG:
    // if boost mode is enabled turn it on
    if (getSettingValue(SettingsOptions::BoostTemp)) {
      cxt->scratch_state.state2 = 1;
    }
    break;
  case BUTTON_F_SHORT:
  case BUTTON_B_SHORT:
    cxt->transitionMode = TransitionAnimation::Left;
    return OperatingMode::TemperatureAdjust;
  case BUTTON_BOTH_LONG:
    if (getSettingValue(SettingsOptions::LockingMode)) {
      // Lock buttons
      if (cxt->scratch_state.state1 == 0) {
        if (warnUser(translatedString(Tr->LockingKeysString), buttons)) {
          cxt->scratch_state.state1 = 2;
        }
      } else {
        // FIXME should be WarningKeysUnlockedString
        warnUser(translatedString(Tr->UnlockingKeysString), buttons);
      }
    }
    break;
  default:
    break;
  }
  return OperatingMode::Soldering;
}

OperatingMode gui_solderingMode(const ButtonState buttons, guiContext *cxt) {
  /*
   * * Soldering (gui_solderingMode)
   * -> Main loop where we draw temp, and animations
   * --> User presses buttons and they goto the temperature adjust screen
   * ---> Display the current setpoint temperature
   * ---> Use buttons to change forward and back on temperature
   * ---> Both buttons or timeout for exiting
   * --> Long hold front button to enter boost mode
   * ---> Just temporarily sets the system into the alternate temperature for
   * PID control
   * --> Long hold back button to exit
   * --> Double button to exit
   * --> Long hold double button to toggle key lock
   */

  // Update the setpoints for the temperature
  if (cxt->scratch_state.state2) {
    if (getSettingValue(SettingsOptions::TemperatureInF)) {
      currentTempTargetDegC = TipThermoModel::convertFtoC(getSettingValue(SettingsOptions::BoostTemp));
    } else {
      currentTempTargetDegC = (getSettingValue(SettingsOptions::BoostTemp));
    }
  } else {
    if (getSettingValue(SettingsOptions::TemperatureInF)) {
      currentTempTargetDegC = TipThermoModel::convertFtoC(getSettingValue(SettingsOptions::SolderingTemp));
    } else {
      currentTempTargetDegC = (getSettingValue(SettingsOptions::SolderingTemp));
    }
  }

  // The reached-temperature signal (beep + blink) fires once per set temperature. Scratch state is cleared on every mode
  // change, so remember it across a trip to the temperature adjust screen: coming back with the same set temperature
  // doesn't replay it, a new set temperature does (once the tip gets there). Any other way in (heat confirm, wake from
  // sleep) is a new heat-up.
  static bool              reachedThisHeatUp = false;
  static TemperatureType_t reachedTargetC    = 0;
  if (!cxt->scratch_state.state4) {
    cxt->scratch_state.state4 = 1;
    if (cxt->previousMode != OperatingMode::TemperatureAdjust || reachedTargetC != currentTempTargetDegC) {
      reachedThisHeatUp = false;
    }
    cxt->scratch_state.state5 = reachedThisHeatUp;
  }

  // Update status
  int  error  = currentTempTargetDegC - TipThermoModel::getTipInC();
  bool atTemp = (error >= -10 && error <= 10);
  if (atTemp) {
    if (!reachedThisHeatUp) {
      reachedThisHeatUp = true;
      reachedTargetC    = currentTempTargetDegC;
    }
    // converged; beep once per soldering session, the first time the set point is reached
    if (!cxt->scratch_state.state5) {
      setBuzzer(true);
      cxt->scratch_state.state3 = xTaskGetTickCount() + TICKS_SECOND / 3;
      cxt->scratch_state.state5 = true;
      // Pinecil V2 has no buzzer, so also blink the screen
      cxt->scratch_state.state6 = 1;
      cxt->scratch_state.state8 = xTaskGetTickCount();
    }
    setStatusLED(LED_HOT);
  } else {
    setStatusLED(LED_HEATING);
  }
  if (cxt->scratch_state.state3 != 0 && xTaskGetTickCount() >= cxt->scratch_state.state3) {
    setBuzzer(false);
  }

  // At-temp screen blink: flip the display invert relative to the user's setting.
  // GUIThread restores the user's setting on any mode change, so leaving mid-blink is safe.
  if (cxt->scratch_state.state6) {
    const bool     userInverted = getSettingValue(SettingsOptions::OLEDInversion);
    const uint32_t phase        = (xTaskGetTickCount() - cxt->scratch_state.state8) / AT_TEMP_BLINK_PHASE;
    if (phase >= AT_TEMP_BLINK_PHASES) {
      cxt->scratch_state.state6 = 0;
      OLED::setInverseDisplay(userInverted);
    } else {
      OLED::setInverseDisplay(userInverted != (phase % 2 == 0));
    }
  }

  // Draw in the screen details
  const int8_t heatEta = heatUpEtaSeconds(error, atTemp);
  if (getSettingValue(SettingsOptions::DetailedSoldering)) {
    ui_draw_soldering_power_status(cxt->scratch_state.state2);
  } else {
    // Set temperature in display units (the boost temperature while boosting)
    const TemperatureType_t target = getSettingValue(cxt->scratch_state.state2 ? SettingsOptions::BoostTemp : SettingsOptions::SolderingTemp);
    ui_draw_soldering_basic_status(cxt->scratch_state.state2, target, heatEta);
  }
  // Underline the tip temperature while it is within +-10 of the set point
  if (atTemp) {
    ui_draw_at_temp_underline(getSettingValue(SettingsOptions::DetailedSoldering));
  }

  bool detailedView = getSettingValue(SettingsOptions::DetailedIDLE) && getSettingValue(SettingsOptions::DetailedSoldering);
  // Check if we should bail due to undervoltage for example
  if (checkExitSoldering()) {
    setBuzzer(false);
    cxt->transitionMode = detailedView ? TransitionAnimation::None : TransitionAnimation::Right;
    return OperatingMode::HomeScreen;
  }
#ifdef NO_SLEEP_MODE

  if (shouldShutdown()) {
    // shutdown
    currentTempTargetDegC = 0;
    cxt->transitionMode   = detailedView ? TransitionAnimation::None : TransitionAnimation::Right;
    return OperatingMode::HomeScreen;
  }
#endif
  if (shouldBeSleeping()) {
    return OperatingMode::Sleeping;
  }

  if (heaterThermalRunawayCounter > 8) {
    currentTempTargetDegC       = 0; // heater control off
    heaterThermalRunawayCounter = 0;
    cxt->transitionMode         = TransitionAnimation::Right;
    return OperatingMode::ThermalRunaway;
  }
  return handleSolderingButtons(buttons, cxt);
}
