#include "stdint.h"

// PIEZO SPEAKER (random part)
// https://github.com/ZulNs/TonePlayer
#define PIEZO_P 3

void speaker_play();
void speaker_init();
void speaker_set_volume(uint8_t volume);
void speaker_loop();
void set_song();