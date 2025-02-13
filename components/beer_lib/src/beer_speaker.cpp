#include "beer_speaker.h"
#include <TonePlayer.h>
#include "beer_led.h"

// const PROGMEM uint8_t SONG_START[] = { NC6, 256-2, ND6, 256-2, NE6,  256-2, NF5,   256-2};
const PROGMEM uint8_t SONG_START[] = {  
    NG5, 256-3, NC6, 256-3, NE6, 256-3, NG6, 256-2, REST, 256-3,
    NE6, 256-3, NG6, 255
};
const PROGMEM uint8_t SONG_FIX[]   = { NC6, 256-4 };
const PROGMEM uint8_t SONG_LONG[]   = { NC6, 256-32 };
const PROGMEM uint8_t SONG_420[]   = {
  NGIS5, 256-32, NGIS5, 256-32, NGIS5, 256-32, NGIS5, 256-32, NGIS5, 256-16, NGIS5, 256-32,
  NGIS5, 256-8,  NGIS5, 256-32, NGIS5, 256-32, NGIS5, 256-32, NG5, 256-16, NF5, 0, 3, 8, 
  NGIS5, 256-32, NGIS5, 256-32, NGIS5, 256-32, NAIS5, 256-32, NAIS5, 256-16, NAIS5, 256-32, 
  NAIS5, 256-32, NAIS5, 256-32, NAIS5, 256-32, NAIS5, 256-8,  NG5, 256-32, NG5, 256-32, //50
  NG5, 256-32, NF5, 256-16, NDIS5, 0, 3, 8, NAIS5, 256-8, NC6, 256-32, NC6, 256-32, 
  NC6, 256-32, NC6, 256-16, NAIS5, 256-16, NGIS5, 256-16, NGIS5, 256-16, NGIS5, 256-32,  
  NDIS6, 256-32, NDIS6, 256-32, NC6, 256-32, NDIS6, 256-32, NCIS6, 256-4, NF6, 0, 3, 32,//90 
  NF6, 256-32, NDIS6, 256-32, NDIS6, 256-32, NCIS6, 256-16, NC6, 0, 3, 32, NC6, 256-32, 
  NCIS6, 256-32, NCIS6, 256-32, NGIS5, 265-16, NAIS5, 0, 3, 32, NGIS5, 256-32,  NAIS5, 256-32, 
  NAIS5, 256-32, NG5, 256-32, NAIS5, 256-16, NGIS5, 0, 3, 16, NDIS6, 256-32, NF6, 256-32,
  NDIS6, 256-16, NCIS6, 256-16, NC6, 256-32, NC6, 256-32, NAIS5, 256-16, REPEAT, 0, 0, 142, 0
};
//RTTTL: x:b=50,o=5,d=32:p,g#,g#,g#,g#,16g#,g#,8g#,g#,g#,g#,16g,4f.,g#,g#,g#,a#,16a#,a#,a#,a#,a#,8a#,g,g,g,16f,4d#.,8a#,c6,c6,c6,16c6,16a#,16g#,16g#,g#,d#6,d#6,c6,d#6,4c#6,16f.6,f6,d#6,d#6,16c#6,16c.6,c6,c#6,c#6,16g#,16a#.,g#,a#,a#,g,16a#,8g#.,d#6,f6,16d#6,16c#6,c6,c6,16a#,

// library seems hardcoded to ledc timer0 channel0, which requires some workaround for display pwm and light sleep mode
TonePlayer piezo(PIEZO_P);

void speaker_loop(){
  piezo.loop();
}

void speaker_set_volume(uint8_t volume){
  piezo.setVolume(volume / 7.0 * 255.0); 
  piezo.setSong((uint8_t*)SONG_FIX, sizeof(SONG_FIX), 169);
  piezo.play();
}

void speaker_play(){
  piezo.play();
}

void set_song(){
  piezo.setSong((uint8_t*)SONG_FIX, sizeof(SONG_FIX), 169);
}

void speaker_init(){
  pinMode(PIEZO_P, OUTPUT);
  piezo.setSong((uint8_t*)SONG_START, sizeof(SONG_START), 175);
  speaker_play();
}