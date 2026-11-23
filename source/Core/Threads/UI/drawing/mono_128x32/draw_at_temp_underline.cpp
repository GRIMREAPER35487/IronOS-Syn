#include "ui_drawing.hpp"

#ifdef OLED_128x32

// Not implemented for the 128x32 layouts (this fork only targets the Pinecil V2).
void ui_draw_at_temp_underline(bool detailedView) { (void)detailedView; }

#endif
