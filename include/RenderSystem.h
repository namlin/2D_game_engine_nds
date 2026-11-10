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

  void update(void) {
    for (auto entity : this->get_entities()) {
      if (!entity.is_alive() ||
          !entity.has_component<SpriteComponent>() ||
          !entity.has_component<TransformComponent>()) {
        continue;
      }

      const auto& sprite = entity.get_component<SpriteComponent>();
      const auto& transform = entity.get_component<TransformComponent>();

      s16 screen_x = static_cast<s16>(transform.position.x);
      s16 screen_y = static_cast<s16>(transform.position.y);

      if (sprite.is_3D) {
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          // Update NFlib 3D sprite position:
          NF_Move3dSprite(sprite.id, screen_x, screen_y);

          if (sprite.is_rotscale && transform.rotation != 0) {
            NF_Rotate3dSprite(sprite.id, 0, 0, transform.rotation);
          }
        }
      } else {
        if (sprite.screen < 2 && sprite.id < 128 && NF_SPRITEOAM[sprite.screen][sprite.id].created) {
          // Update NFlib 2D sprite position (hardware sprite coordinates):
          // sprite.screen: 0 (Top screen) or 1 (Bottom screen)
          // sprite.id: NFlib hardware sprite slot ID (0 to 127)
          NF_MoveSprite(sprite.screen, sprite.id, screen_x, screen_y);

          // RotScale handling (if entity uses rotation or scaling):
          if (sprite.is_rotscale) {
            // NFlib rotscale matrices use 8-bit fixed precision (256 = 1.0f scale)
            s16 scale_x = transform.scale.x >> 4;  // Convert 20.12 fixed down to 256-base
            s16 scale_y = transform.scale.y >> 4;

            // Apply rotation angle (0-511) and scale to assigned matrix slot:
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
};

#endif  // RENDERSYSTEM_H
