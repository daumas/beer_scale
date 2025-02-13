#include "beer_rotary.h"
#include "beer_led.h"
#include "beer_display.h"
#include "beer_speaker.h"
#include <Preferences.h>
#include "beer_strain_gauge.h"


// library seems unstable (v1.7) but can handle pulldown encoder with interrupt, high active button not working, handled manually
AiEsp32RotaryEncoder rotary = AiEsp32RotaryEncoder(ROTARY_A, ROTARY_B, -1, -1, 4, false);

uint8_t btn_hist = 0;   //   last button states f. debouncing
bool buttonPressed = false;
unsigned long pressStartTime = 0;
bool longPressDetected = false;

#define LONG_PRESS_TIME     10000

// CODE
void IRAM_ATTR readEncoderISR(){
    rotary.readEncoder_ISR();
}

void rotary_init(){
  // INIT ROTARY
  rotary.begin();
  rotary.setup(readEncoderISR);
  rotary.setAcceleration(0);
  rotary.correctionOffset=0;
  pinMode(ROTARY_T, INPUT_PULLUP);
  //pinMode(ROTARY_A, INPUT_PULLDOWN);
  //pinMode(ROTARY_B, INPUT_PULLDOWN);

  rotary.setBoundaries(0, 7, false);
  rotary.setEncoderValue(7);
}

void rotary_menu(long maxVal, bool circleVal, long setVal){
  rotary.setBoundaries(0, maxVal, circleVal);
  rotary.setEncoderValue(setVal);
}

long rotary_read(){
  return rotary.readEncoder();
}

void rotary_check(){
  if (rotary.encoderChanged()){
    draw_menu(rotary.readEncoder(), 0);
  }
  
  if (digitalRead(ROTARY_T) == LOW && !buttonPressed) {
    delay(50);  // Simple debounce
    if (digitalRead(ROTARY_T) == LOW) {
      draw_menu(rotary.readEncoder(), 1);
      buttonPressed = true;
      pressStartTime = millis();
      longPressDetected = false;
    }
  } else if (digitalRead(ROTARY_T) == LOW && !longPressDetected) {
    // Button is still being held
    if (millis() - pressStartTime >= LONG_PRESS_TIME)
    {
      // Long press detected
      longPressDetected = true;

      Preferences preferences = get_preferences();
      preferences.begin("storage", false);  // Namespace "storage", false = Read/Write mode
      float measurement = scale_single_measure();
      int cal = (500 / measurement) * 1000;   // LONG PRESS CALIBRATES TO 500g
      preferences.putInt("cal", cal);
      preferences.end();
      set_cal(cal);
      
      set_song();
      speaker_play();
    }
  } else if (digitalRead(ROTARY_T) == HIGH && buttonPressed) {
    buttonPressed = false;
  }

}

//------------------------------------

const int buttonPin = 19;    // Switch on GPIO0
const int encoderPinA = 34;  // Encoder A on GPIO1
const int encoderPinB = 35;  // Encoder B on GPIO2

int aLastState;
//bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const int debounceDelay = 200;  // Adjust as needed
uint8_t led = 12;

void enc_init() {
  pinMode(buttonPin, INPUT_PULLUP);       // Switch uses internal pull-up
  pinMode(encoderPinA, INPUT_PULLDOWN);   // Encoder pins use pull-down
  pinMode(encoderPinB, INPUT_PULLDOWN);
  aLastState = digitalRead(encoderPinA);
}

void check_encoder() {
  // Detect button press (active LOW)
  if (digitalRead(buttonPin) == LOW && !buttonPressed) {
    delay(50);  // Simple debounce
    if (digitalRead(buttonPin) == LOW) {
      led_set_color(10, 0, 255, 0);
      buttonPressed = true;
    }
  } else if (digitalRead(buttonPin) == HIGH && buttonPressed) {
    buttonPressed = false;  // Reset on release
    led_set_color(10, 0, 0, 0);
  }

  // Detect encoder rotation
  int aState = digitalRead(encoderPinA);
  if (aState != aLastState) {
    if (millis() - lastDebounceTime > debounceDelay) {
      int bState = digitalRead(encoderPinB);
      if (aState != bState) {
        led++;
        led_set_color(led, 0, 255, 0);
      } else {
        led--;
        led_set_color(led, 0, 0, 255);
      }
      lastDebounceTime = millis();
    }
    aLastState = aState;  // Update state after debounce
  }
}
