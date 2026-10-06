#ifndef VEC2_H
#define VEC2_H

#include <nds.h>

// 20.12 Fixed-point 2D Vector (NDS Hardware Optimized):
struct Vec2f {
  s32 x;
  s32 y;

  Vec2f(s32 x = 0, s32 y = 0) : x(x), y(y) {}

  // Convenience constructor for standard floats:
  Vec2f(float fx, float fy)
        : x(floatToFixed(fx, 12)), y(floatToFixed(fy, 12)) {}

  // Basic operator overloads:
  Vec2f operator+(const Vec2f& rhs) const {
    return Vec2f(x + rhs.x, y + rhs.y);
  }

  Vec2f operator*(s32 scalar) const {
    return Vec2f((x * scalar) >> 12, (y * scalar) >> 12);
  }
};

#endif // VEC2_H
