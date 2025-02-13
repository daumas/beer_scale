// Set chip in bootloader mode -> erase flash via connecting 2 serial wires &:
// esptool.py --chip esp32 erase_flash
// Then program via:
// openocd -f board/esp32-wrover-kit-3.3v.cfg -c "program_esp build/bootloader/bootloader.bin verify exit 0x1000"
// openocd -f board/esp32-wrover-kit-3.3v.cfg -c "program_esp build/partition_table/partition-table.bin verify exit 0x8000"
// openocd -f board/esp32-wrover-kit-3.3v.cfg -c "program_esp build/beer_scale.bin 0x10000 verify exit reset"

// openocd -f board/esp32-wrover-kit-3.3v.cfg
// flash erase_address 0 0x400000 or flash erase_sector 0 0 100


// openocd -f board/esp32-wrover-kit-3.3v.cfg -c "program_esp build/beer_scale.bin 0x10000 verify exit reset"
#include "Adafruit_GFX.h"
#include "Adafruit_ST7735.h"
#include "AiEsp32RotaryEncoder.h"
#include "ESP32_WS2812_Lib.h"
#include "HX711.h"
#include "beer_display.h"
#include "beer_led.h"
#include "beer_speaker.h"
#include "beer_strain_gauge.h"
#include "beer_rotary.h"
#include "beer_wifi.h"
#include "config.h"
#include "esp32-hal-adc.h"
#include "esp32-hal-gpio.h"
#include "esp_timer.h"
//#include "driver/timer.h"

#define BAT_L 5
#define BAT_T 5
#define BAT_V_MAX 4200
#define BAT_V_MIN 3000

volatile bool timerTriggered = false;
volatile int led_ctr = 4;
esp_timer_handle_t led_timer;

void measureBattery();

static void battery_timer_callback(void* arg){
    timerTriggered = true;  // Set the flag to true when the timer is triggered
}

static void led_timer_callback(void *arg){
  led_set_color(led_ctr, 255, 0, 0);
  led_set_color(led_ctr-1, 200, 0, 0);
  led_set_color(led_ctr-2, 100, 0, 0);
  led_set_color(led_ctr-3, 30, 0, 0);
  led_set_color(led_ctr-4, 0, 0, 0);
  led_show();
  led_ctr++;
}

void setup()
{
  led_init();
  
  // LED animation
  const esp_timer_create_args_t led_timer_args = {
            .callback = &led_timer_callback,
            .name = "led"
  };
  
  ESP_ERROR_CHECK(esp_timer_create(&led_timer_args, &led_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(led_timer, 37000));

  gfx_init();
  display_init();
  draw_main();

  // set timers 1,2 to use low speed rc clock which runs during light sleep
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
                                    .timer_num = LEDC_TIMER_1, .freq_hz = 100, .clk_cfg = LEDC_USE_RC_FAST_CLK};
  ledc_timer_config(&ledc_timer);
  ledc_timer.timer_num = LEDC_TIMER_2;
  ledc_timer_config(&ledc_timer);
  
  scale_init();
  rotary_init();

  // Charge Measurement
  pinMode(GPIO_NUM_1, OUTPUT);
  const esp_timer_create_args_t battery_timer_args = {
            .callback = &battery_timer_callback,
            .name = "battery"
  };
  esp_timer_handle_t battery_timer;
  ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(battery_timer, 10000000));
  
  measureBattery();
  init_brightness();
  speaker_init();
  
  connect();
  
  ESP_ERROR_CHECK(esp_timer_stop(led_timer));
  ESP_ERROR_CHECK(esp_timer_delete(led_timer));
  for (uint8_t i=0; i<24; i++){
    led_set_color(i, 0, 0, 0);
  }
}

void loop()
{
  if (timerTriggered && get_menu() == 0) {
    timerTriggered = false;  // Reset the flag
    measureBattery();        // Call the battery measurement function
  }

  speaker_loop(); // keeps the song playing when started

  // measure and average
  measure();
  esp_now_routine();

  // draw main - menu only drawn on input
  if(get_menu() == 0){
    txt_draw();
    stable_draw();
    gfx_draw();
    beer_draw();
  }
  
  rotary_check();

  // frame rate limiter, use sleep to save some battery if possible
  if(33 - min(33lu, millis() - get_tlast()) > 0){
    if(1)//!sleep_en)
      delay(33 - min(33lu, millis() - get_tlast()));
    else{
      gpio_wakeup_enable(ROTARY_A, GPIO_INTR_HIGH_LEVEL);
      gpio_wakeup_enable(ROTARY_B, GPIO_INTR_HIGH_LEVEL);
      gpio_wakeup_enable(ROTARY_T, GPIO_INTR_HIGH_LEVEL);
      esp_sleep_enable_gpio_wakeup();
      if(get_peer_found() || get_peer_start()){
        esp_now_deinit();
        WiFi.mode(WIFI_OFF);
      }
      esp_sleep_enable_timer_wakeup(1000*(33-min(33lu, millis() - get_tlast())));
      esp_light_sleep_start(); 
      if(get_peer_found() || get_peer_start()) {
        connect();
      }
    }
  }
  uint32_t tnow = millis();

  set_tlast(tnow);
}


void measureBattery(){
  digitalWrite(GPIO_NUM_1, HIGH);
  delay(10);
  int voltage = analogReadMilliVolts(36) * 110 / 10;
  int charge = ((voltage - BAT_V_MIN) *100) / (BAT_V_MAX - BAT_V_MIN);
  if (charge < 1){
    charge = 1;
  }
  if (charge > 100){
    charge = 100;
  }
  digitalWrite(GPIO_NUM_1, LOW);
  char txt[12];
  sprintf(txt, "%d%%", charge);
  tft_print_all_it(txt, BAT_L, BAT_T, 1);
}