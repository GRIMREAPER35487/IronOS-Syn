#include "OperatingModes.h"
#include "ui_drawing.hpp"

/*
 * Heat confirm screen (custom, not in upstream IronOS)
 *
 * Shown when the iron-side button is pressed on the home screen, instead of heating straight away.
 * The heater stays OFF the whole time this screen is up.
 *
 *  - Iron-side (front) button, double press  -> start Soldering mode (heats)
 *  - Minus (back) button, short press        -> open the set temperature adjust screen, then come back here
 *  - Minus (back) button, hold               -> back to the home screen
 *  - No buttons for HEAT_CONFIRM_TIMEOUT     -> back to the home screen
 *  - Tip removed                             -> back to the home screen
 */

#define HEAT_CONFIRM_DOUBLE_WINDOW (TICKS_100MS * 6) // Max time between the two presses (release to release)
#define HEAT_CONFIRM_TIMEOUT       (TICKS_SECOND * 10)

OperatingMode gui_heatConfirm(const ButtonState buttonIn, guiContext *cxt) {
  currentTempTargetDegC = 0; // Never heat on this screen

  uint16_t   *waitForRelease = &(cxt->scratch_state.state1);
  uint16_t   *firstPressSeen = &(cxt->scratch_state.state2);
  uint32_t   *firstPressTime = &(cxt->scratch_state.state3);
  ButtonState buttons        = buttonIn;

  // Ignore anything still held from whatever got us here, until all buttons are released
  if (*waitForRelease == 0) {
    if (buttons != BUTTON_NONE) {
      buttons = BUTTON_NONE;
    } else {
      *waitForRelease = 1;
    }
  }

  const TickType_t now = xTaskGetTickCount();

  // First press expired without a second one: disarm
  if (*firstPressSeen && (now - *firstPressTime) > HEAT_CONFIRM_DOUBLE_WINDOW) {
    *firstPressSeen = 0;
  }

  if (isTipDisconnected()) {
    cxt->transitionMode = TransitionAnimation::Right;
    return OperatingMode::HomeScreen;
  }

  switch (buttons) {
  case BUTTON_F_SHORT:
    if (*firstPressSeen) {
      // Second press inside the window: go heat
      cxt->transitionMode = TransitionAnimation::None;
      return OperatingMode::Soldering;
    }
    *firstPressSeen = 1;
    *firstPressTime = now;
    break;
  case BUTTON_B_SHORT:
    cxt->transitionMode = TransitionAnimation::Left;
    return OperatingMode::TemperatureAdjust; // Returns to cxt->previousMode, i.e. back here
  case BUTTON_B_LONG:
    cxt->transitionMode = TransitionAnimation::Right;
    return OperatingMode::HomeScreen;
  default:
    // F long, both buttons etc. are ignored on purpose
    break;
  }

  // Timeout counts from entering the screen or the last button activity, whichever is later
  TickType_t lastActivity = lastButtonTime > cxt->viewEnterTime ? lastButtonTime : cxt->viewEnterTime;
  if ((now - lastActivity) > HEAT_CONFIRM_TIMEOUT) {
    cxt->transitionMode = TransitionAnimation::Right;
    return OperatingMode::HomeScreen;
  }

  // Blink the iron icon quickly once the first press is in, so you can see it is waiting for the second
  bool showIcon = !(*firstPressSeen) || ((now / TICKS_100MS) % 2 == 0);
  ui_draw_heat_confirm(showIcon);

  return OperatingMode::HeatConfirm;
}
