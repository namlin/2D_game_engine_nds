#ifndef VEC2_H
#define VEC2_H

#include <nds.h>

struct Vec2f {
  s32 x;
  s32 y;

  constexpr Vec2f(s32 xValue = 0, s32 yValue = 0)
      : x(xValue), y(yValue) {}

  static Vec2f fromFloat(float fx, float fy) {
    return Vec2f(
      floatToFixed(fx, 12),
      floatToFixed(fy, 12)
    );
  }

  Vec2f operator+(const Vec2f& rhs) const {
    return Vec2f(x + rhs.x, y + rhs.y);
  }

  Vec2f operator*(s32 scalar) const {
    return Vec2f(
      (x * scalar) >> 12,
      (y * scalar) >> 12
    );
  }
};

#endif
