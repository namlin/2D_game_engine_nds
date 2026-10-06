#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include <filesystem.h>
#include <nds.h>
#include <nf_lib.h>

struct SpriteComponent {
    u8 screen;       // 0 = Top screen, 1 = Bottom screen
    u8 id;           // NFlib allocated sprite slot ID (0 - 127)
    u16 gfx_id;      // Graphics ID loaded in VRAM
    u16 palette_id;  // Palette ID loaded in VRAM
    u16 width;
    u16 height;
    bool is_rotscale;

    SpriteComponent(u8 screen = 0, u8 id = 0, u16 width = 32, u16 height = 32,
                    bool is_rotscale = false)
        : screen(screen), id(id), gfx_id(0), palette_id(0), width(width),
          height(height), is_rotscale(is_rotscale) {}
};

#endif // SPRITECOMPONENT_H
