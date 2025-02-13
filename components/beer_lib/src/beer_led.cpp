#include "beer_led.h"

ESP32_WS2812 leds = ESP32_WS2812(24, LED_DI, 0);

void led_init(){
  leds.begin();
}

void led_set_color(int index, uint8_t r, uint8_t g, uint8_t b){
  leds.setLedColor(index % 24, r, g, b);
}

void led_show(){
  leds.show();
}
