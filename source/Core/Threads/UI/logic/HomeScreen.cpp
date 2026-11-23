
#include "Buttons.hpp"
#include "OperatingModes.h"
#include "ui_drawing.hpp"

bool showExitMenuTransition = false;

// Safe-to-touch indicator (custom). Below 43 C skin can't burn even with long contact; 38 C leaves sensor margin.
#define SAFE_TO_TOUCH_C 38                 // tip must drop below this to count as safe
#define HOT_C           40                 // at or above this the tip counts as hot again (2 C hysteresis)
#define SAFE_BADGE_TIME (TICKS_SECOND * 5) // how long the SAFE badge shows on the simple home screen

HomeCooldownState homeCooldown = {false, false, false};

// Scratch: state2 = cooling down, state5 = cooled to safe, state3 = tick it became safe (state1 is button handling)
static void updateCooldownState(uint32_t tipTempC, guiContext *cxt) {
  uint16_t        *coolingDown = &(cxt->scratch_state.state2);
  uint16_t        *cooledSafe  = &(cxt->scratch_state.state5);
  uint32_t        *safeSince   = &(cxt->scratch_state.state3);
  const TickType_t now         = xTaskGetTickCount();

  if (isTipDisconnected()) {
    *coolingDown = 0;
    *cooledSafe  = 0;
  } else if (tipTempC >= HOT_C) {
    *coolingDown = 1;
    *cooledSafe  = 0;
  } else if (*coolingDown && tipTempC < SAFE_TO_TOUCH_C) {
    *coolingDown = 0;
    *cooledSafe  = 1;
    *safeSince   = now;
  }

  homeCooldown.coolingDown  = *coolingDown;
  homeCooldown.safeDetailed = *cooledSafe;
  homeCooldown.safeSimple   = *cooledSafe && ((now - *safeSince) < SAFE_BADGE_TIME);
}

OperatingMode handleHomeButtons(const ButtonState buttons, guiContext *cxt) {
  if (buttons != BUTTON_NONE && cxt->scratch_state.state1 == 0) {
    return OperatingMode::HomeScreen; // Ignore button press
  } else {
    cxt->scratch_state.state1 = 1;
  }
  switch (buttons) {
  case BUTTON_NONE:
    // Do nothing
    break;
  case BUTTON_BOTH:
    break;

  case BUTTON_B_LONG:
    cxt->transitionMode = TransitionAnimation::Down;
    return OperatingMode::DebugMenuReadout;
    break;
  case BUTTON_F_LONG:
#ifdef PROFILE_SUPPORT
    if (!isTipDisconnected()) {
      cxt->transitionMode = TransitionAnimation::Left;
      return OperatingMode::SolderingProfile;
    } else {
      return OperatingMode::HomeScreen;
    }
#else
    cxt->transitionMode = TransitionAnimation::Left;
    return OperatingMode::TemperatureAdjust;
#endif
    break;
  case BUTTON_F_SHORT:
    if (!isTipDisconnected()) {
      // Don't heat straight away: show the set temperature and wait for a double press first
      cxt->transitionMode = TransitionAnimation::Left;
      return OperatingMode::HeatConfirm;
    }
    break;
  case BUTTON_B_SHORT:
    cxt->transitionMode = TransitionAnimation::Right;
    return OperatingMode::SettingsMenu;
    break;
  default:
    break;
  }
  return OperatingMode::HomeScreen;
}

OperatingMode drawHomeScreen(const ButtonState buttons, guiContext *cxt) {

  currentTempTargetDegC = 0; // ensure tip is off
  getInputVoltageX10(getSettingValue(SettingsOptions::VoltageDiv), 0);
  uint32_t tipTemp = TipThermoModel::getTipInC();
  updateCooldownState(tipTemp, cxt);

  // Setup LCD Cursor location
  if (OLED::getRotation()) {
    OLED::setCursor(50, 0);
  } else {
    OLED::setCursor(-1, 0);
  }
  if (getSettingValue(SettingsOptions::DetailedIDLE)) {
    ui_draw_homescreen_detailed(tipTemp);
  } else {
    ui_draw_homescreen_simplified(tipTemp);
  }
  return handleHomeButtons(buttons, cxt);
}
