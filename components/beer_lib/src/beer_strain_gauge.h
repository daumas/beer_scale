#include "HX711.h"
#include <Preferences.h>

// SCALE (Aliexpress 10KG Load Cell with green HX711 board)
// https://github.com/RobTillaart/HX711
#define SCALE_MISO  27//12
#define SCALE_CLK   26//14

// tara sign
#define TAR_L 159 - 6*3 - 4 //137 to 154
#define TAR_T 52  

#define STABLE_t_1 2200   //ms consider value stable

float get_avg();
float get_stable();
uint8_t get_bri_led();
void set_bri_led(uint8_t val);
uint8_t get_disp_tar();
void set_disp_tar(uint8_t val);
uint8_t get_stable_avg();
float get_avg_l();
Preferences get_preferences();
void set_cal(int cal);

float scale_single_measure();
void scale_init();
void measure();
