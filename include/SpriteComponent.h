#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include <filesystem.h>
#include <nds.h>
#include <nf_lib.h>

struct SpriteComponent {
  std::string gfx_name;  // Base filename without extension.
  u8 screen;  // 0 = Top Screen, 1 = Bottom Screen.
  u8 sprite_id;  // NFlib Sprite ID (0 to 127).
  u16 width;  // Frame width.
  u16 height;  // Frame height.
  s16 x;  // Screen X position.
  s16 y;  // Screen Y position.

  SpriteComponent(const std::string& gfx_name = "", u8 screen = 0,
                  u8 sprite_id = 0, u16 width = 0, u16 height = 0,
                  s16 x = 0, s16 y = 0)
      : gfx_name(gfx_name),
        screen(screen),
        sprite_id(sprite_id),
        width(width),
        height(height),
        x(x),
        y(y) {}
};

#endif // SPRITECOMPONENT_H
