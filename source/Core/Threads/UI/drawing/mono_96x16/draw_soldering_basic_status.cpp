#include "SolderingCommon.h"
#include "power.hpp"
#include "ui_drawing.hpp"
#ifdef OLED_96x16

// Basic soldering screen (custom layout for this fork):
//
//   |320   [ 318 °C ]   12s|      top row:    set temperature  |  heat-up countdown, or how far the tip is off target
//    45W                 20V      bottom row: live watts       |  input voltage
//
// Each side block is 24px wide and the live temperature keeps its stock place (x = 24), so the at-temp underline still lines up.
// A short bar on the outer edge marks the top row of each block. In left handed mode the two side blocks swap over.
// Bottom-row text sits at y = 9 so there is always one blank row between the two lines of small text.

#define BLOCK_TEXT_LEFT  2  // text x of the left block (bar at x = 0)
#define BLOCK_TEXT_RIGHT 77 // text x of the right block (3 small chars end at x = 94, bar at x = 95)
#define BOTTOM_ROW_Y     9

static void drawEdgeBar(int16_t x) { OLED::drawFilledRect(x, 0, x, 6, false); }

// Three characters, right aligned: " +2", "-15", "  0"
static void printOffTarget(int32_t off) {
  if (off > 99) {
    off = 99;
  } else if (off < -99) {
    off = -99;
  }
  const uint16_t magnitude = off < 0 ? -off : off;
  if (magnitude < 10) {
    OLED::print(SmallSymbolSpace, FontStyle::SMALL);
  }
  if (off == 0) {
    OLED::print(SmallSymbolSpace, FontStyle::SMALL);
  } else {
    OLED::print(off > 0 ? SmallSymbolPlus : SmallSymbolMinus, FontStyle::SMALL);
  }
  OLED::printNumber(magnitude, magnitude < 10 ? 1 : 2, FontStyle::SMALL);
}

void ui_draw_soldering_basic_status(bool boostModeOn, TemperatureType_t targetTemp, int8_t heatEtaSeconds) {
  (void)boostModeOn; // while boosting, the caller passes the boost temperature as the target
  const bool    leftHanded = OLED::getRotation();
  const int16_t targetX    = leftHanded ? BLOCK_TEXT_RIGHT : BLOCK_TEXT_LEFT;
  const int16_t statusX    = leftHanded ? BLOCK_TEXT_LEFT : BLOCK_TEXT_RIGHT;

  // Set temperature, with its edge bar
  OLED::setCursor(targetX, 0);
  OLED::printNumber(targetTemp, 3, FontStyle::SMALL);
  drawEdgeBar(leftHanded ? OLED_WIDTH - 1 : 0);

  // Live watts
  OLED::setCursor(targetX, BOTTOM_ROW_Y);
  const uint32_t watts = (x10WattHistory.average() + 5) / 10;
  if (watts > 99) {
    OLED::printNumber(watts, 3, FontStyle::SMALL); // no room for the W above 99
  } else {
    OLED::printNumber(watts, 2, FontStyle::SMALL);
    OLED::print(SmallSymbolWatts, FontStyle::SMALL);
  }

  // Live tip temperature (read once: the display filter updates on every read)
  const TemperatureType_t tipTemp = getTipTemp();
  OLED::setCursor(24, 0);
  OLED::printNumber(tipTemp, 3, FontStyle::LARGE);
  OLED::printSymbolDeg(FontStyle::EXTRAS);

  // Heat-up countdown, or how far the tip is from the target once that's done
  OLED::setCursor(statusX, 0);
  if (heatEtaSeconds >= 0) {
    OLED::printNumber(heatEtaSeconds, 2, FontStyle::SMALL);
    OLED::print(SmallSymbolSeconds, FontStyle::SMALL);
  } else {
    printOffTarget((int32_t)tipTemp - (int32_t)targetTemp);
  }
  drawEdgeBar(leftHanded ? 0 : OLED_WIDTH - 1);

  // Input voltage
  OLED::setCursor(statusX, BOTTOM_ROW_Y);
  const uint32_t volts = (getInputVoltageX10(getSettingValue(SettingsOptions::VoltageDiv), 0) + 5) / 10;
  OLED::printNumber(volts, 2, FontStyle::SMALL);
  OLED::print(SmallSymbolVolts, FontStyle::SMALL);
}
#endif
