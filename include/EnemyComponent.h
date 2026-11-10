#ifndef ENEMYCOMPONENT_H
#define ENEMYCOMPONENT_H

#include <nds.h>

struct EnemyComponent {
  s32 speed;
  bool homing;

  EnemyComponent(s32 speed = 50, bool homing = true) : speed(speed), homing(homing) {}
};

#endif  // ENEMYCOMPONENT_H
