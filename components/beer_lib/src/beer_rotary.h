#include "AiEsp32RotaryEncoder.h"

// ROTARY ENCODER/BUTTON (AliExpress EC11 15mm plum handle, https://de.aliexpress.com/item/1005005983134515.html)
// https://github.com/igorantolic/ai-esp32-rotary-encoder
#define ROTARY_A GPIO_NUM_34
#define ROTARY_B GPIO_NUM_35
#define ROTARY_T GPIO_NUM_19


void rotary_init();
void rotary_menu(long maxVal, bool circleVal, long setVal);
long rotary_read();
void rotary_check();

void enc_init();
void check_encoder();