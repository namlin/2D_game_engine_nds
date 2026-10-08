#ifndef ENEMYCOMPONENT_H
#define ENEMYCOMPONENT_H

#include <nds.h>

struct EnemyComponent {
  s32 speed;

  EnemyComponent(s32 speed = 50) : speed(speed) {}
};

#endif  // ENEMYCOMPONENT_H
