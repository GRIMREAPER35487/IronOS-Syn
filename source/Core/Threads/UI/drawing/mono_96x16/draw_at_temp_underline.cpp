#include "ui_drawing.hpp"

#ifdef OLED_96x16

// 1px line on the bottom row (y = 15) under the three large tip-temperature digits.
// The large digits only use rows 0..13, so row 14 stays as a gap above the line.
// Each digit cell is 12px wide with a blank first column, so the line starts one pixel in.
void ui_draw_at_temp_underline(bool detailedView) {
  uint8_t x0;
  if (detailedView) {
    // ui_draw_soldering_power_status: temp drawn at x = 50 (rotated) or x = -1
    x0 = OLED::getRotation() ? 51 : 0;
  } else {
    // ui_draw_soldering_basic_status: two 12px cells before the temp in both orientations
    x0 = 25;
  }
  const uint8_t x1 = x0 + 34; // to the end of the third digit
  OLED::drawFilledRect(x0, 15, x1, 15, false);
}

#endif
