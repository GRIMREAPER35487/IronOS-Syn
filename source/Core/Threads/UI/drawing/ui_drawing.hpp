#ifndef UI_DRAWING_UI_DRAWING_HPP_
#define UI_DRAWING_UI_DRAWING_HPP_
#include "Buttons.hpp"
#include "OLED.hpp"
#include "OperatingModeUtilities.h"
#include "Settings.h"
#include "configuration.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
void ui_draw_warning_undervoltage(void);
void ui_draw_power_source_icon(void);                            // Draw a single character wide power source icon
void ui_draw_tip_temperature(bool symbol, const FontStyle font); // Draw tip temp, aware of conversions
bool warnUser(const char *warning, const ButtonState buttons);   // Print a full screen warning to the user
void ui_draw_cjc_sampling(const uint8_t num_dots);               // Draws the CJC info text and progress dots
void ui_draw_debug_menu(const uint8_t item_number);              // Draws the debug menu state
void ui_draw_homescreen_detailed(TemperatureType_t tipTemp);     // Drawing the home screen -- Detailed mode
void ui_draw_homescreen_simplified(TemperatureType_t tipTemp);   // Drawing the home screen -- Simple mode

// Safe-to-touch state for the home screen, worked out each frame in HomeScreen.cpp (custom)
struct HomeCooldownState {
  bool coolingDown;  // tip has been hot and is not below the safe temperature yet: keep showing the temp
  bool safeSimple;   // just became safe: show the SAFE badge on the simple home screen (for a few seconds)
  bool safeDetailed; // cooled from hot to safe: show the OK badge on the detailed home screen (while cool)
};
extern HomeCooldownState homeCooldown;

void ui_draw_safe_badge(bool smallBadge, int16_t x); // 96x16 only: lit box with SAFE (40px) or OK (18px)

void ui_pre_render_assets(void); // If any assets need to be pre-rendered into ram
// Soldering mode
void ui_draw_soldering_power_status(bool boost_mode_on);
// targetTemp is in display units; heatEtaSeconds is the heat-up countdown, or -1 to show how far the tip is off target instead
void ui_draw_soldering_basic_status(bool boostModeOn, TemperatureType_t targetTemp, int8_t heatEtaSeconds);
void ui_draw_at_temp_underline(bool detailedView); // 1px line under the tip temp while at the set point
void ui_draw_soldering_detailed_sleep(TemperatureType_t tipTemp);
void ui_draw_soldering_basic_sleep(TemperatureType_t tipTemp);
void ui_draw_soldering_profile_advanced(TemperatureType_t tipTemp, TemperatureType_t profileCurrentTargetTemp, uint32_t phaseElapsedSeconds, uint32_t phase, const uint32_t phaseTimeGoal);

// Temp change
void ui_draw_temperature_change(void);
// Heat confirm screen: iron icon (hidden when showIcon is false, used to blink) + set temperature
void ui_draw_heat_confirm(bool showIcon);
// USB-PD debug
void ui_draw_usb_pd_debug_state(const uint16_t vbus_sense_state, const uint8_t stateNumber);
void ui_draw_usb_pd_debug_pdo(const uint8_t entry_num, const uint16_t min_voltage, const uint16_t max_voltage, const uint16_t current_a_x100, const uint16_t wattage);
// Utils
void printVoltage(void);
#endif // UI_DRAWING_UI_DRAWING_HPP_
