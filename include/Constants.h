#ifndef CONSTANTS_H
#define CONSTANTS_H

const size_t MAXSPRITES = 8;
const size_t MAX_COMPONENTS = 64;
const size_t POOL_SIZE = 1000;
const size_t EXTRA_SIZE = 100;

// On NDS, 1 frame at 60 FPS takes ~0.01668 seconds.
constexpr double TARGET_DELTA_TIME = 1.0 / 59.826;

#endif  // CONSTANTS_H
