#include "ESP32_WS2812_Lib.h"

// WS2812 LED RING, 24 LEDs
// https://github.com/Zhentao-Lin/ESP32_WS2812_Lib
#define LED_DI 2


void led_init();
void led_set_color(int index, uint8_t r, uint8_t g, uint8_t b);
void led_show();
