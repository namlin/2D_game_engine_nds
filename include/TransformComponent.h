#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include <nds.h>

#include "Vec2.h"

struct TransformComponent {
  Vec2f position;
  Vec2f scale;
  s16 rotation;  // Rotation angle (0 - 511 or 0 - 359 degrees).

  TransformComponent(Vec2f pos = Vec2f(0, 0),
             Vec2f scl = Vec2f(floatToFixed(1.0f, 12), floatToFixed(1.0f, 12)),
                       s16 rot = 0)
    : position(pos), scale(scl), rotation(rot) {}
};

#endif // TRANSFORMCOMPONENT_H
