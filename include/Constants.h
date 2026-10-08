#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <nds.h>
#include <cstdint>

const uint8_t FPS = 60;
const uint16_t MILLISECS_PER_FRAME = 1000 / FPS;
const size_t MAXSPRITES = 8;
const size_t MAX_COMPONENTS = 64;
const size_t POOL_SIZE = 1000;
const size_t EXTRA_SIZE = 100;

// On NDS, 1 frame at 60 FPS takes ~0.01668 seconds.
constexpr double TARGET_DELTA_TIME = 1.0 / 59.826;

// Sprite dimensions (SCREEN_WIDTH and SCREEN_HEIGHT are defined in libnds):
const uint16_t PLAYER_WIDTH = 64;
const uint16_t PLAYER_HEIGHT = 64;

const uint16_t MENACE_WIDTH = 32;
const uint16_t MENACE_HEIGHT = 32;

const uint16_t BULLET_WIDTH = 8;
const uint16_t BULLET_HEIGHT = 8;

// Variables:
extern u32 global_frame_counter ;

extern u16 keys;  // Keys currently pressed.
extern touchPosition touchscreen;

#endif  // CONSTANTS_H
