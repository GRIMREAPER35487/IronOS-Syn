#include "ui_drawing.hpp"

#ifdef OLED_96x16
extern uint8_t buttonAF[sizeof(buttonA)];

// [ iron icon 0..41 ][   set temp, large font   ]   (mirrored when the screen is rotated)
void ui_draw_heat_confirm(bool showIcon) {
  if (OLED::getRotation()) {
    if (showIcon) {
      OLED::drawArea(54, 0, 42, 16, buttonAF);
    }
    OLED::setCursor(2, 1);
  } else {
    if (showIcon) {
      OLED::drawArea(0, 0, 42, 16, buttonA);
    }
    OLED::setCursor(48, 1);
  }
  // y = 1: the large digits only use glyph rows 0..13, so drawing one row down centres them (rows 1..14)
  // on the 16px panel. drawChar() handles the non strip-aligned y by shifting each column.
  OLED::printNumber(getSettingValue(SettingsOptions::SolderingTemp), 3, FontStyle::LARGE);
  OLED::printSymbolDeg(FontStyle::EXTRAS);
}
#endif
