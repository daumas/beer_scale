#include "beer_strain_gauge.h"
#include "beer_display.h"
#include "beer_wifi.h"
#include "beer_led.h"
#include <Preferences.h>

HX711 scale;

float raw[5] = {0};     //g  weight raw value buffer
uint8_t pos = 0;        //g  weight raw value buffer position
float davg = 0;         //g  weight difference
float tar = 0;          //g  tara value   fir/iir filtered below 2g
uint32_t stable_ms = 0;   //ms time without change
uint8_t BR8_temp[] = {0, 1, 2, 5,  15, 45,  100, 255};

// GETTER
float avg = 0;          //g  weight       fir/iir filtered
float stable = 0;      //   ratio of stable time to STABLE_t_1
uint8_t bri_led = 5;    //   0..7
uint8_t disp_tar = 0;   //   display tare changing, boolean
int16_t stable_avg = 0;      //g  stable value, full integers
float avg_l = 0;        //g  weight, last
int cal = 0;

Preferences preferences;

float get_avg(){return avg;}
float get_stable(){return stable;}
uint8_t get_bri_led(){return bri_led;}
void set_bri_led(uint8_t val){bri_led = val;}
uint8_t get_disp_tar(){return disp_tar;}
void set_disp_tar(uint8_t val){disp_tar = val;}
uint8_t get_stable_avg(){return stable_avg;}
float get_avg_l(){return avg_l;}
Preferences get_preferences(){return preferences;}
void set_cal(int cal_n){cal = cal_n;};

void scale_init(){
    scale.begin(SCALE_MISO, SCALE_CLK);
    scale.set_scale(220);      // //n:222.22 //b:220.56
    tar = scale.get_units(10); // bad first value?
    avg = tar = scale.get_units(10);
    
    preferences.begin("storage", false);  // Namespace "storage", false = Read/Write mode
    cal = preferences.getInt("cal", 1000);  // Default = 0 if not found
    preferences.end();
}

float scale_single_measure(){
  return (scale.get_units(100) - tar);
}

// weight calculation, tar update, stability update ----------
void measure(){
  // read value
  raw[pos] = (scale.get_units(1) - tar) * float(cal)/1000;
  #ifdef DEBUG
    Serial.printf("raw: %fg\n", raw[pos]);
  #endif
  pos = (pos+1) % 5;

  // fir moving average/outlier filter
  float vmin = 1e7, vmax = -1e7, val = 0;
  for(int i=0; i<5; i++){
    if(raw[i] < vmin) vmin = raw[i];
    if(raw[i] > vmax) vmax = raw[i];
    val += raw[i];
  }
  val = (val - vmin - vmax) / 3;
  
  // iir ema with increased sample weight for large differences
  davg = 0.9 * davg + 0.1 * (val - avg);  //g change
  float dnoise = 0.5;                     //g expected sample standard deviation in davg
  float alpha = 0.009 + 0.180 * ((fabs(davg)/dnoise))/((fabs(davg)/dnoise)+1);
  
  //float delta = millis()-stable_ms;
  //float alpha = 0.01 + 0.15 * max(0.0f, min(1.0f, (float)STABLE_t_0 * STABLE_t_0 / delta / delta ));
  //if(fabs(avg) < 2) alpha = 0.04;
  avg_l = avg;
  avg = (1-alpha) * avg + alpha * val;

  // iir tare below 2g to account for beer sweat on scale
  if(fabs(avg) < 10.0 && fabs(val) < 10.0){
    tar = tar + 0.01 * val;
    if(disp_tar == 0){
      tft_print_all_it("TAR", TAR_L, TAR_T, 1);
      for(uint8_t i=0; i<24; i++)
        led_set_color(i, 0, 0, BR8_temp[bri_led] / 32); // blue
      led_show();
      send_weight();
      disp_tar = 1;
    }
  }
  else{
    if(disp_tar == 1){
      tft_print_all_it("   ", TAR_L, TAR_T, 1);
      for(uint8_t i=0; i<24; i++)
        led_set_color(i, BR8_temp[bri_led] / 2, BR8_temp[bri_led] / 2, 0); // yellow
      led_show();
      disp_tar = 0;
    }
  }

  // reset stable time if value changed
  // based on rate of change
  uint16_t sta = (uint16_t)floor(round(avg*10)/10);
  if((fabs(avg) < 10.0) || fabs(davg) > 0.5){
    stable_ms = millis();
    stable_avg = sta;
  }
  stable = float(millis() - stable_ms) / STABLE_t_1;
}