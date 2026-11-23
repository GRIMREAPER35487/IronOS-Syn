#include "ui_drawing.hpp"

#ifdef OLED_128x32
extern uint8_t buttonAF[sizeof(buttonA)];

// [ iron icon 0..55 ][   set temp, large font   ]   (mirrored when the screen is rotated)
void ui_draw_heat_confirm(bool showIcon) {
  if (OLED::getRotation()) {
    if (showIcon) {
      OLED::drawArea(72, 0, 56, 32, buttonAF);
    }
    OLED::setCursor(4, 8);
  } else {
    if (showIcon) {
      OLED::drawArea(0, 0, 56, 32, buttonA);
    }
    OLED::setCursor(64, 8);
  }
  OLED::printNumber(getSettingValue(SettingsOptions::SolderingTemp), 3, FontStyle::LARGE);
  OLED::printSymbolDeg(FontStyle::EXTRAS);
}
#endif
