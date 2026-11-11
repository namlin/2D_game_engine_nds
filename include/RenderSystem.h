#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include <nds.h>
#include <nf_lib.h>

#include "SpriteComponent.h"
#include "System.h"
#include "TransformComponent.h"

class RenderSystem : public System {
 public:
  RenderSystem(void) {
    this->require_component<SpriteComponent>();
    this->require_component<TransformComponent>();
  }

  ~RenderSystem(void) = default;

  void update(s32 camera_x = 0, s32 camera_y = 0) {
    for (auto entity : this->get_entities()) {
      if (!entity.is_alive() ||
          !entity.has_component<SpriteComponent>() ||
          !entity.has_component<TransformComponent>()) {
        continue;
      }

      const auto& sprite = entity.get_component<SpriteComponent>();
      const auto& transform = entity.get_component<TransformComponent>();

      s32 world_x = transform.position.x - camera_x;
      s32 world_y = transform.position.y - camera_y;

      bool is_visible = (world_x + static_cast<s32>(sprite.width) > 0) &&
                        (world_x < static_cast<s32>(SCREEN_WIDTH)) &&
                        (world_y + static_cast<s32>(sprite.height) > 0) &&
                        (world_y < static_cast<s32>(SCREEN_HEIGHT));

      s16 screen_x = static_cast<s16>(world_x);
      s16 screen_y = static_cast<s16>(world_y);

      if (sprite.is_3D) {
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          NF_Show3dSprite(sprite.id, is_visible);

          if (is_visible) {
            // Update NFlib 3D sprite position:
            NF_Move3dSprite(sprite.id, screen_x, screen_y);

            if (sprite.is_rotscale && transform.rotation != 0) {
              NF_Rotate3dSprite(sprite.id, 0, 0, transform.rotation);
            }
          }
        }
      } else {
        if (sprite.screen < 2 && sprite.id < 128 && NF_SPRITEOAM[sprite.screen][sprite.id].created) {
          NF_ShowSprite(sprite.screen, sprite.id, is_visible);

          if (is_visible) {
            // Update NFlib 2D sprite position (hardware sprite coordinates):
            NF_MoveSprite(sprite.screen, sprite.id, screen_x, screen_y);

            // RotScale handling (if entity uses rotation or scaling):
            if (sprite.is_rotscale) {
              s16 scale_x = transform.scale.x >> 4;
              s16 scale_y = transform.scale.y >> 4;

              NF_SpriteRotScale(
                sprite.screen,
                sprite.id,
                transform.rotation,
                scale_x,
                scale_y
              );
            }
          }
        }
      }
    }
  }
};

#endif  // RENDERSYSTEM_H
