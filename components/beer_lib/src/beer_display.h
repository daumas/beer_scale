#pragma once
#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>


// DISPLAY (Aliexpress 1.77" TFT)
// https://github.com/adafruit/Adafruit-ST7735-Library
#define TFT_CS0     -1 // manual
#define TFT_CS1     16
#define TFT_CS2     17
#define TFT_CS3     21
#define TFT_CS4     22
#define TFT_RST     -1 // reset on reset
#define TFT_DC      5

#define TXT_L 10
#define TXT_T 52

#define BGSCALE ST77XX_BLACK
#define FGSCALE ST77XX_WHITE
#define SCALE_L 0
#define SCALE_R 160
#define SCALE_T 128-41
#define SCALE_B 128-8
#define SCALE_M 80       //px "center" of scale for calculation
#define ZOOM    3          //px/g zoom factor for scale width

#define BEER_T  4
#define BEER_L  10

// get stable value
#define STABLE_L   5    
#define STABLE_T   45 
#define STABLE_W   150
#define STABLE_H   3 
#define STABLE_t_0 200    //ms start displaying bar 700

#define BUBBLE_N    20      //   number of bubbles

uint32_t get_tlast();
void set_tlast(uint32_t tlast);
uint8_t get_menu();
float get_avg_rec();
float get_stable_rec();

void display_init();
void init_brightness();
void draw_main();
void beer_init();
void gfx_init();
void txt_draw();
void stable_draw();
void beer_draw();
void gfx_draw();
void draw_menu(uint8_t v, uint8_t click);
uint16_t gfx_atox(float midval, float a);
float gfx_xtoa(float midval, float x);
void gfx_draw_text(float midval, uint16_t color);
void tft_print_all_it(const __FlashStringHelper* s, int16_t x, int16_t y, uint8_t sz);
void tft_print_all_it(const char* s, int16_t x, int16_t y, uint8_t sz);
void tft_print_all(const __FlashStringHelper* s, int16_t x, int16_t y);
void tft_print_all(int16_t v, int16_t x, int16_t y);
void tft_cs_all(uint8_t val);