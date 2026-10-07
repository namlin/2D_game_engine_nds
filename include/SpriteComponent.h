#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include <filesystem.h>
#include <nds.h>
#include <nf_lib.h>

struct SpriteComponent {
    u8 screen;
    u8 id;
    u16 gfx_id;
    u16 palette_id;
    u16 width;
    u16 height;
    bool is_3D;
    bool is_rotscale;

    SpriteComponent(
        u8 screen = 0,
        u8 id = 0,
        u16 gfx_id = 0,
        u16 palette_id = 0,
        u16 width = 32,
        u16 height = 32,
        bool is_3D = false,
        bool is_rotscale = false
    )
        : screen(screen),
          id(id),
          gfx_id(gfx_id),
          palette_id(palette_id),
          width(width),
          height(height),
          is_3D(is_3D),
          is_rotscale(is_rotscale) {}
};

#endif // SPRITECOMPONENT_H
