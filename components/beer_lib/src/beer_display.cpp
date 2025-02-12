#include "beer_display.h"
#include "driver/ledc.h"
#include "beer_led.h"
#include "beer_gfx.h"
#include "beer_wifi.h"
#include "beer_strain_gauge.h"
#include "beer_rotary.h"
#include "beer_speaker.h"
#include <stdio.h>

uint8_t TFT_BL[5] = {0, 4, 25, 32, 33};
// MOSI, SCK using hardware VSPI 23/18
/*Adafruit_ST7735 tfts[5] = {{TFT_CS0, TFT_DC, TFT_RST}, // fake tft, used to print on all
                           {TFT_CS1, TFT_DC, TFT_RST},
                           {TFT_CS2, TFT_DC, TFT_RST},
                           {TFT_CS3, TFT_DC, TFT_RST},
                           {TFT_CS4, TFT_DC, TFT_RST}};*/

// text
uint16_t FG[5] = {ST77XX_BLACK, ST77XX_BLACK, ST77XX_WHITE, ST77XX_WHITE, ST77XX_BLACK};
uint16_t BG[5] = {ST77XX_WHITE, ST77XX_GREEN, ST77XX_RED,   ST77XX_BLUE,  ST77XX_YELLOW};
uint16_t BR10[] = {1, 3, 8, 20, 60, 180, 400, 1023};
uint8_t BR8[] = {0, 1, 2, 5,  15, 45,  100, 255};
uint8_t bri_tft = 7;    //   0..7

// scale display
uint8_t gfx_h[100] = {0};  //px scale line heights, indexed in rounded g

// beer display
uint16_t BEER_m_0[3]  = {16,  380, 345};  //g  mass of empty beer
uint16_t BEER_m_1[3]  = {516, 880, 845};  //g  mass of full beer
uint8_t BEER_x_1[3]   = {85,  112, 100};  //px x of full beer
uint8_t BEER_x_M[3]   = {95,  140, 120};  //px array length
uint16_t BEER_c_w[3]  = {0x8C51, 0x0000, 0x7161};  //   wall color
uint16_t BEER_c_e[3]  = {0xCE37, 0xCB20, 0xBAE4};  //   empty color
uint16_t BEER_c_b[3]  = {0xFE21, 0xFCC2, 0xE487};  //   beer color
uint16_t BEER_c_f[3]  = {0xFFFF, 0xEE96, 0xEDF2};  //   foam color
uint8_t beer_disp = 255;//   0..can, 1..nrw, 2..eur, .., 255..disable
uint8_t beer_x_l = 0;   //px last drawn x value 
uint8_t *beer_gfx;      //   points to correct beer graphic
uint16_t *beer_gfx_img; //   points to correct beer graphic bitmap

float disp_avg = 0;     //g  weight, remote or local avg
float disp_stable = 0;  //g  weight, remote or local avg
float disp_avg_l = 0;   //g  weight, remote or local avg
uint8_t  stable_x_l = 0;  //px last drawn x value
uint8_t  stable_xled_l = 0;//px last drawn x value on leds
uint8_t disp_fix = 0;   //   display fix, boolean
uint8_t volume = 7;     //   0..7 -> 0..255 ()

// bubbles
uint8_t bubble_x[BUBBLE_N] = {0}; //px x positions of bubbles
uint8_t bubble_y[BUBBLE_N] = {0}; //px y positions ob bubbles


// GETTER
uint32_t tlast;         //ms loop timing
uint8_t menu = 0;       //   which menu we are in, 0=none
float avg_rec = 0;      //g  weight, received from other scale
float stable_rec = 0;   //   ratio of stable time to STABLE_t_1, received

uint32_t get_tlast(){return tlast;}
void set_tlast(uint32_t val){tlast = val;}
uint8_t get_menu(){return menu;}
float get_avg_rec(){return avg_rec;}
float get_stable_rec(){return stable_rec;}


#define NUM_DISPLAYS 5
#define SPI_SPEED 20000000

Adafruit_ST7735 tfts[NUM_DISPLAYS] = {
    Adafruit_ST7735(-1, 5, -1),
    Adafruit_ST7735(16, 5, -1),  // Example CS, DC, RST pins
    Adafruit_ST7735(17, 5, -1),
    Adafruit_ST7735(21, 5, -1),
    Adafruit_ST7735(22, 5, -1)
};

volatile bool displayReady[NUM_DISPLAYS] = {false};  // Track init status

void initDisplay(void *param) {
    int i = (int)param;  
    tfts[i].setSPISpeed(SPI_SPEED);
    tfts[i].initR(INITR_BLACKTAB);
    tfts[i].setRotation(3);
    tfts[i].setTextWrap(false);
    if (i > 0) {
      tfts[i].setTextColor(FG[i], BG[i]);
      tfts[i].setTextSize(3);
    } else {
      tfts[i].setTextColor(FG[i]);
      tfts[i].setTextSize(1);
    }
    displayReady[i] = true;  // Mark as initialized
    vTaskDelete(NULL);
}

void waitForDisplays() {
    bool allReady = false;
    while (!allReady) {
        allReady = true;
        for (int i = 0; i < NUM_DISPLAYS; i++) {
            if (!displayReady[i]) {
                allReady = false;
                break;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Short delay to avoid busy-waiting
    }
}

void display_init()
{
    for (int i = 0; i < NUM_DISPLAYS; i++) {
      xTaskCreate(initDisplay, "InitTask", 4096, (void *)i, 1, NULL);
    }

    waitForDisplays();

    pinMode(TFT_CS1, OUTPUT);
    pinMode(TFT_CS2, OUTPUT);
    pinMode(TFT_CS3, OUTPUT);
    pinMode(TFT_CS4, OUTPUT);
    tft_cs_all(HIGH);
    
    // INIT BACKLIGHT
    for (uint8_t i = 1; i < NUM_DISPLAYS; i++)
    {
        pinMode(TFT_BL[i], OUTPUT);
        ledcAttachChannel(TFT_BL[i], 100, 10, 9 + i); // 10,11 -> T1 12,13 -> T2
        //ledcWrite(TFT_BL[i], BR10[bri_tft]);
        //led_set_color(i, 0, 255, 0);
    }
/*
    // INIT DISPLAYS
    pinMode(TFT_CS1, OUTPUT);
    pinMode(TFT_CS2, OUTPUT);
    pinMode(TFT_CS3, OUTPUT);
    pinMode(TFT_CS4, OUTPUT);
    tft_cs_all(HIGH);

    for (uint8_t i = 0; i < 5; i++)
    {
        tfts[i].setSPISpeed(10000000);
        tfts[i].initR(INITR_BLACKTAB);
        tfts[i].setRotation(3);
        tfts[i].setTextWrap(false);
        if (i > 0)
        {
            tfts[i].setTextColor(FG[i], BG[i]);
            tfts[i].setTextSize(3);
        }
        else
        {
            tfts[i].setTextColor(FG[i]);
            tfts[i].setTextSize(1);
        }
    }
    */
}

void init_brightness(){
  for(uint8_t i=1; i<5; i++)
    ledcWrite(TFT_BL[i], BR10[bri_tft]);
}

void draw_main(){
  menu = 0;
  
  for(uint8_t i = 1; i < 5; i++)
    tfts[i].fillScreen(BG[i]);

  // draw scale background
  tft_cs_all(LOW);
  tfts[0].fillRect(0, 128-48, 160, 48, BGSCALE);      
  tfts[0].drawFastHLine(0, 128-49, 160, FGSCALE);
  tfts[0].drawFastVLine(SCALE_M, 128-48, 19, ST77XX_RED);
  tfts[0].drawFastVLine(SCALE_M, 128-20, 20, ST77XX_RED);
  tft_cs_all(HIGH);

  tlast = millis();

  // draw fixed text
  tft_print_all_it("    . g", TXT_L, TXT_T, 3);

  // draw placeholder image
  beer_disp = 255;
  for(uint8_t i = 1; i < 5; i++)
    tfts[i].fillRect(0, 0, 160, BEER_T + 36, BG[i]);

  uint16_t* c = (uint16_t*)BEER_LOGO_IMG;
  tft_cs_all(LOW);
  for(uint8_t y = 2; y < 44; y++)
    for(uint8_t x = 160-70; x < 160-70+64; x++){
      if(*c != 0xF81F) //magenta
        tfts[0].writePixel(x, y, *c);
      c++;
    }

  tfts[0].fillRect(6, 20, 160-70-6, 2, 0x047a);
  tft_cs_all(HIGH);

  if(beer_disp != 255)
    beer_init();
}

// beer draw functions ----------
void beer_init(){
  // clear whole top area, individual displays
  for(uint8_t i = 1; i < 5; i++)
    tfts[i].fillRect(0, 0, 160, BEER_T + 40, BG[i]);

  // draw bottle/can top half border from beer_gfx array
  #define hhalf 18
  #define twall 3
  uint8_t h;
  tft_cs_all(LOW);
  for(uint8_t x = 0; x < BEER_x_M[beer_disp]; x++){
    if(x < twall || x > (BEER_x_M[beer_disp] - twall -1))
      h = hhalf - beer_gfx[x];  // full wall, no beer
    else
      h = twall;
    tfts[0].drawFastVLine(BEER_L + x, BEER_T + beer_gfx[x], h, BEER_c_w[beer_disp]);
    tfts[0].drawFastVLine(BEER_L + x, BEER_T + beer_gfx[x] + h, hhalf - beer_gfx[x] - h, BEER_c_e[beer_disp]);
  }    
  tft_cs_all(HIGH);

  // copy bottle/can bottom half from beer_gfx_img bitmap
  uint16_t* c = beer_gfx_img;
  tft_cs_all(LOW);
  for(uint8_t y = BEER_T + 18; y < BEER_T + 36; y++){
    for(uint8_t x = BEER_L; x < BEER_L + BEER_x_M[beer_disp]; x++){
      if(*c != 0xF81F) //magenta
        tfts[0].writePixel(x, y, *c);
      c++;
    }
  }
  tft_cs_all(HIGH);
}

// pre calculate height of scale lines
void gfx_init(){
  for(uint8_t i = 0; i<100; i++){
    if(i % 20 == 0)       gfx_h[i] = 8;
    else if(i % 10 == 0)  gfx_h[i] = 12;
    else if(i % 2 == 0)   gfx_h[i] = 3;
  }
}

void draw_menu(uint8_t v, uint8_t click){
  if(click){ // button pressed, new menu
    switch(menu){
      case 0:
        // 0 -> 1: initialize menu
        for(uint8_t i=1; i<5; i++)
          tfts[i].fillScreen(BG[i]);
        tft_print_all_it(" back...", 16, 10, 2);
        tft_print_all_it(" link...", 16, 10+16, 2);
        tft_print_all_it(" led bri.", 16, 10+32, 2);
        tft_print_all_it(" tft bri.", 16, 10+48, 2);
        tft_print_all_it(" snd vol.", 16, 10+64, 2);
        menu = 1;
        rotary_menu(4, true, 0);
        v = 0;
        break;
      case 1:
        // 1 -> x: select
        if(v == 0){
          draw_main(); // includes menu=0, encoder stuff
          return;
        }
        if(v == 1){
          connect();
          draw_main();
          return;
        }
        if(v == 2){
          menu = 12;
          tft_print_all_it("=", 10, 10+2*16, 2);
          rotary_menu(7, false, get_bri_led());
        }
        if(v == 3){
          menu = 13;
          tft_print_all_it("=", 10, 10+3*16, 2);
          rotary_menu(7, false, bri_tft);
        }
        if(v == 4){
          menu = 14;
          tft_print_all_it("=", 10, 10+4*16, 2);
          rotary_menu(7, false, volume);
        }
        break;
      case 12:
        menu = 1;
        rotary_menu(4, true, 2);
        break;
      case 13:
        menu = 1;
        rotary_menu(4, true, 3);
        break;
      case 14:
        menu = 1;
        rotary_menu(4, true, 4);
        break;
    }
    v = rotary_read();
  }

  // encoder input
  //if(menu == 0){
  //  if(bri_led == bri_tft) // change both if same
  //    bri_led = v;
  //  bri_tft = v;
  //  ledcWrite(TFT_BL[1], BR10[v]);
  //  ledcWrite(TFT_BL[2], BR10[v]);
  //  ledcWrite(TFT_BL[3], BR10[v]);
  //  ledcWrite(TFT_BL[4], BR10[v]);
  //}

  if(menu == 12 || menu == 0){
    set_bri_led(v);
    for(uint8_t i=0; i<24; i++)
      led_set_color(i, 0, BR8[get_bri_led()], 0);
    led_show();
  }
  if(menu == 13){
    bri_tft = v;
    ledcWrite(TFT_BL[1], BR10[v]);
    ledcWrite(TFT_BL[2], BR10[v]);
    ledcWrite(TFT_BL[3], BR10[v]);
    ledcWrite(TFT_BL[4], BR10[v]); 
  }
  if(menu == 14){
    volume = v;
    speaker_set_volume(volume);
  }

  // draw
  if(menu == 1){
    for(uint8_t i=0; i<5; i++){
      if(i == v) 
        tft_print_all_it(">", 10, 10+i*16, 2);
      else 
        tft_print_all_it(" ", 10, 10+i*16, 2);
    }
  }
  if(menu == 1 || menu == 12){
    char txt[4];
    sprintf(txt, "%d", get_bri_led());
    tft_print_all_it(txt, 130, 42, 2);  
  }
  if(menu == 1 || menu == 13){
    char txt[4];
    sprintf(txt, "%d", bri_tft);
    tft_print_all_it(txt, 130, 58, 2);  
  }
  if(menu == 1 || menu == 14){
    char txt[4];
    sprintf(txt, "%d", volume);
    tft_print_all_it(txt, 130, 74, 2);  
  }
}



// draws the weight, local or remote
void txt_draw(){
  char txt[19];

  disp_avg = get_avg();
  disp_stable = get_stable();
  uint8_t bri_led = get_bri_led();

  if(get_peer_found()){
    if(avg_rec > 10.0 && get_disp_tar()){
      if(get_disp_remote() != 1){
        tft_print_all_it(" ", 4, TAR_T+8, 1);
        tft_print_all_it("R", 4, TAR_T+16, 1);
        set_disp_remote(1);
        for(uint8_t i=0; i<24; i++)
          led_set_color(i, BR8[bri_led] / 2, BR8[bri_led] / 2, 0); // yellow
        led_show();
      }
      else
        // otherwise remote avg not drawn
        disp_stable = stable_rec; 
      disp_avg = avg_rec;
    }
    else{
      if(get_disp_remote() != 0){
        tft_print_all_it("L", 4, TAR_T+8, 1);
        tft_print_all_it(" ", 4, TAR_T+16, 1);
        set_disp_remote(0);
        set_disp_tar(0); //reprints TAR
      }
    }
  }

  if(round(10*disp_avg) == round(10*disp_avg_l)) 
    return;

  if(disp_avg <= -1000)
    sprintf(txt, "low");
  else{
    if(disp_avg > 4000)
      sprintf(txt, "high");
    else{
      sprintf(txt, "%6.1fg%6.1fg", get_avg(), avg_rec);
    }
  }

  if(disp_stable < 1)
    tft_print_all_it(txt, TXT_L, TXT_T, 3);
  disp_avg_l = disp_avg;
}

// draws the stable line,  leds and FIX/420 text ----------
void stable_draw(){
  uint8_t bri_led = get_bri_led();
  // bar
  uint8_t x = max(0.0f, min((float)STABLE_W, STABLE_W * disp_stable));
  for(uint8_t i=1; i<5; i++){
    if(x > stable_x_l){
      tfts[i].fillRect(STABLE_L + stable_x_l, STABLE_T, x - stable_x_l, STABLE_H, FG[i]);
    }
    else if (x < stable_x_l){ // reset
      tfts[i].fillRect(STABLE_L + x,          STABLE_T, stable_x_l - x, STABLE_H, BG[i]);
    }
  }

  // same for leds
  uint8_t xled = max(0.0f, min((float)25, 25 * disp_stable));
  if(xled > stable_xled_l){
    for(uint8_t i=stable_xled_l; i<xled; i++){
        //leds.setLedColorData(i, int((5-i)/5.0*255), int(i/5.0*255), 0);
        led_set_color(i, BR8[bri_led] / 2, 0, BR8[bri_led] / 2); // purple
        }
     led_show();
  }
  else if (x < stable_x_l){ // reset
    for(uint8_t i=0; i<24; i++)
        led_set_color(i, BR8[bri_led] / 2, BR8[bri_led] / 2, 0); // yellow
    led_show();
  }

  // logos
  if(disp_stable >=1 ){
    if(disp_fix == 0){
      tft_print_all_it("FIX", TAR_L, TAR_T, 1);
      for(uint8_t i=0; i<24; i++)
        led_set_color(i, 0, BR8[bri_led], 0); // green
          led_show();
      disp_fix = 1;
      set_song();
      //piezo.setSong((uint8_t*)SONG_FIX, sizeof(SONG_FIX), 169);
      //if(stable_avg == 420){
      //  tft_print_all_it("420", TAR_L, TAR_T+8, 1);
      //  piezo.setSong((uint8_t*)SONG_420, sizeof(SONG_420), 12);
      //}
      speaker_play();
    }
  }
  else{ 
    if(disp_fix == 1){
      tft_print_all_it("   ", TAR_L, TAR_T, 1);
      tft_print_all_it("   ", TAR_L, TAR_T+8, 1);
      disp_fix = 0;
    }
  }
  stable_x_l = x;
  stable_xled_l = xled;
}



void beer_draw(){
  #define tol 15  //g  tolerance mass, bottles and filling varies
  uint8_t stable_avg = get_stable_avg();

  // if no beer type specified, either recognize if possible and init or leave
  if(beer_disp == 255){
    if(!disp_fix)
      return;
    if(abs(stable_avg - BEER_m_1[0]) < tol){
      beer_disp = 0; // can
      beer_gfx = (uint8_t*)BEER_CAN_BRD;
      beer_gfx_img = (uint16_t*)BEER_CAN_IMG;
      tft_print_all_it("CAN", TAR_L, TAR_T+16, 1);
    }else if(abs(stable_avg - BEER_m_1[1]) < tol){
      beer_disp = 1; // bottle nrw
      beer_gfx = (uint8_t*)BEER_NRW_BRD;
      beer_gfx_img = (uint16_t*)BEER_NRW_IMG;
      tft_print_all_it("NRW", TAR_L, TAR_T+16, 1);
    }else if(abs(stable_avg - BEER_m_1[2]) < tol){
      beer_disp = 2; // bottle eu
      beer_gfx = (uint8_t*)BEER_EUR_BRD;
      beer_gfx_img = (uint16_t*)BEER_EUR_IMG;
      tft_print_all_it("EUR", TAR_L, TAR_T+16, 1);
    }else return;
    beer_init();
  }

  #define hhalf    18
  #define twall    3
  #define wfoam    3      //px foam width
  #define BEER_x_0 twall  //px start of content
  #define mfull    (BEER_m_1[beer_disp] - BEER_m_0[beer_disp])
  #define xfull    (BEER_x_1[beer_disp] - BEER_x_0)
  
  // draw beer, overwrite affected area
  float mbeer = get_avg() - BEER_m_0[beer_disp];
  uint8_t beer_x = min((float)BEER_x_1[beer_disp], max((float)BEER_x_0, BEER_x_0 + xfull * mbeer / mfull));
  beer_x_l = max(beer_x_l, (uint8_t)BEER_x_0);
  if(beer_x < beer_x_l){
    tft_cs_all(LOW);
    // less beer, overwrite from last foam top to new foam top with empty
    for(uint8_t x = beer_x_l + wfoam - 1; x > beer_x + wfoam - 1; x--)
      tfts[0].drawFastVLine(BEER_L + x, BEER_T + beer_gfx[x] + twall, hhalf - twall - beer_gfx[x], BEER_c_e[beer_disp]);
    // less beer, overwrite from new foam top to new beer top with foam
    for(uint8_t x = beer_x + wfoam - 1; x > beer_x - 1; x--)
      tfts[0].drawFastVLine(BEER_L + x, BEER_T + beer_gfx[x] + twall, hhalf - twall - beer_gfx[x], BEER_c_f[beer_disp]);
    tft_cs_all(HIGH);
  }
  else if(beer_x > beer_x_l){
    tft_cs_all(LOW);
    // more beer, overwrite from last beer top to new beer top with beer
    for(uint8_t x = beer_x_l; x < beer_x; x++)
      tfts[0].drawFastVLine(BEER_L + x, BEER_T + beer_gfx[x] + twall, hhalf - twall - beer_gfx[x], BEER_c_b[beer_disp]);
    // more beer, overwrite from new beer top to new foam top with foam
    for(uint8_t x = beer_x; x < beer_x + wfoam; x++)
      tfts[0].drawFastVLine(BEER_L + x, BEER_T + beer_gfx[x] + twall, hhalf - twall - beer_gfx[x], BEER_c_f[beer_disp]);
    tft_cs_all(HIGH);
  }

  beer_x_l = beer_x;

  // draw bubbles, remove old, calculate and draw new
  tft_cs_all(LOW);
  for(uint8_t b = 0; b < BUBBLE_N; b++){
    if(bubble_y[b] < hhalf && bubble_x[b] >= twall && bubble_x[b] < beer_x)
      tfts[0].writePixel(BEER_L + bubble_x[b], BEER_T + bubble_y[b], BEER_c_b[beer_disp]);
    if(bubble_y[b] < hhalf && bubble_x[b]+1 >= twall && bubble_x[b]+1 < beer_x)
      tfts[0].writePixel(BEER_L + bubble_x[b]+1, BEER_T + bubble_y[b], BEER_c_b[beer_disp]);
    bubble_x[b] += 1;// - random(0, 10) / 7; // 7/10 chance of rising
    bubble_y[b] += random(-6, 7) / 6; // rounds to zero -> 2/13 chance of 1px y movement
    // bubble at wall -> follow wall (ignores "bottom" wall)
    bubble_y[b] = max((int)bubble_y[b], beer_gfx[bubble_x[b]] + twall);
    // bubble hit max beer -> new bubble, limit was beer_x (foam), but this keeps bubble density constant
    if(bubble_x[b] >= BEER_x_1[beer_disp] || bubble_x[b] < twall){
      bubble_x[b] = random(twall, beer_x/3*2); // don't start at the foam
      bubble_y[b] = random(beer_gfx[bubble_x[b]] + twall, 2*hhalf - twall - beer_gfx[bubble_x[b]]);
    }
    if(bubble_y[b] < hhalf && bubble_x[b] >= twall && bubble_x[b] < beer_x)
      tfts[0].writePixel(BEER_L + bubble_x[b], BEER_T + bubble_y[b], BEER_c_f[beer_disp]);
    if(bubble_y[b] < hhalf && bubble_x[b]+1 >= twall && bubble_x[b]+1 < beer_x)
      tfts[0].writePixel(BEER_L + bubble_x[b]+1, BEER_T + bubble_y[b], 0b1100011000011000);
  }
  tft_cs_all(HIGH);

}

// scale draw functions ----------

void gfx_draw(){

  int16_t a_x, a_x_l;
  uint8_t changed = 0;
  uint8_t h, h_l;

  // iterate all x, calculate corresponding mass and array index for previous and current measurement, look up and draw
  for(int16_t x = SCALE_L; x < SCALE_R; x++){
      if(x == (uint8_t)SCALE_M) continue; // center line here
      a_x   = round(gfx_xtoa(get_avg()  , x+0.5)); // get mass at pixel center, round to full g
      a_x_l = round(gfx_xtoa(get_avg_l(), x+0.5));
      if(changed == 0 && x > ZOOM) return; // no change at all -> leave
      if(a_x == a_x_l) continue; // no change for this x, skip
      changed++;
      uint8_t h   = gfx_h[(a_x   + 10000) % 100];
      uint8_t h_l = gfx_h[(a_x_l + 10000) % 100];
      // new line longer, draw fg from old to new length
      if(h > h_l){
        tft_cs_all(LOW);
        tfts[0].drawFastVLine(x, SCALE_T + h_l, h - h_l, FGSCALE);
        tfts[0].drawFastVLine(x, SCALE_B - h,   h - h_l, FGSCALE);
        tft_cs_all(HIGH);
      }
      // old line longer, draw bg from new to old length
      else if(h_l > h){
        tft_cs_all(LOW);
        tfts[0].drawFastVLine(x, SCALE_T + h,   h_l - h, BGSCALE);
        tfts[0].drawFastVLine(x, SCALE_B - h_l, h_l - h, BGSCALE);
        tft_cs_all(HIGH);
      }
  }
  gfx_draw_text(get_avg_l(), BGSCALE);
  gfx_draw_text(get_avg(),   FGSCALE);
}

// display x position corresponding to array index / mass 
// pixel values are defined to be at the left edge of the pixel
// ZOOM=1 midval=5g SCALE_M=80.5: [4.500g, 5.500g[ -> x=80 (int)
// ZOOM=2 midval=5g SCALE_M=80:   [4.500g, 5.000g[ -> x=80 (int)
// ZOOM=3 midval=5g SCALE_M=80.5: [4.833g, 5.133g[ -> x=80 (int)
uint16_t gfx_atox(float midval, float a){
  return floor(SCALE_M + (a - midval) * ZOOM);
}

// array index / avg mass corresponding to display x value,
// pixel values are defined to be at the left edge of the pixel
// ZOOM=1 midval=5g SCALE_M=80.5:  x=79 -> 3.5g   / x=80 -> 4.5g   / x=81 -> 5.5g
// ZOOM=2 midval=5g SCALE_M=80:    x=79 -> 4.5g   / x=80 -> 5g     / x=81 -> 5.5g
// ZOOM=3 midval=5g SCALE_M=80.5:  x=79 -> 4.5g   / x=80 -> 4.833g / x=81 -> 5.133g / x=82 -> 5.5
float gfx_xtoa(float midval, float x){
  return midval + (x - SCALE_M) / ZOOM;
}

void gfx_draw_text(float midval, uint16_t color){
  // TEXT
  #define diff 20
  int16_t v = (int16_t)((midval + 10000) / diff - 1) * diff - 10000;
  int16_t x = gfx_atox(midval, v);
  uint8_t textshift = 0;
  for(uint8_t i = 0; i < 4; i++){
    if(v > 999)         textshift = 9;
    else if(v > 99)     textshift = 6;
    else if(v > 9)      textshift = 3;
    else if(v > -1)     textshift = 0;
    else if(v > -10)    textshift = 3;
    else if(v > -100)   textshift = 6;
    else if(v > -1000)  textshift = 9;
    else if(v > -10000) textshift = 12;

    tfts[0].setTextColor(color);
    tft_print_all(v, x - 2 - textshift, ((uint16_t)SCALE_T + SCALE_B) / 2 - 3);
    v += diff;
    x += diff * ZOOM;
  }
}









// tft helper functions ----------
void tft_print_all_it(const __FlashStringHelper* s, int16_t x, int16_t y, uint8_t sz){
  for(uint8_t i=1; i<5; i++){
    tfts[i].setTextSize(sz);
    tfts[i].setCursor(x, y);
    tfts[i].print(s);
  }
}
void tft_print_all_it(const char* s, int16_t x, int16_t y, uint8_t sz){
  for(uint8_t i=1; i<5; i++){
    tfts[i].setTextSize(sz);
    tfts[i].setCursor(x, y);
    tfts[i].print(s);
  }
}
void tft_print_all(const __FlashStringHelper* s, int16_t x, int16_t y){
  tft_cs_all(LOW);
  tfts[0].setCursor(x, y);
  tfts[0].print(s);
  tft_cs_all(HIGH);
}
void tft_print_all(int16_t v, int16_t x, int16_t y){
  tft_cs_all(LOW);
  tfts[0].setCursor(x, y);
  tfts[0].print(v);
  tft_cs_all(HIGH);
}
void tft_cs_all(uint8_t val){
  digitalWrite(TFT_CS1, val);
  digitalWrite(TFT_CS2, val);
  digitalWrite(TFT_CS3, val);
  digitalWrite(TFT_CS4, val);
}