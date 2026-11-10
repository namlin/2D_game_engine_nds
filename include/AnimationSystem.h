#ifndef ANIMATIONSYSTEM_H
#define ANIMATIONSYSTEM_H

#include <algorithm>
#include <nds.h>
#include <nf_lib.h>

#include "../include/Constants.h"

// ECS:
#include "../include/System.h"

// Components:
#include "../include/AnimationComponent.h"
#include "../include/SpriteComponent.h"

class AnimationSystem : public System {
 public:
  AnimationSystem(void) {
    this->require_component<AnimationComponent>();
    this->require_component<SpriteComponent>();
  }

  void update(void) {
    u32 current_ticks = global_frame_counter;

    for (auto entity : this->get_entities()) {
      if (!entity.is_alive() ||
          !entity.has_component<AnimationComponent>() ||
          !entity.has_component<SpriteComponent>()) {
        continue;
      }

      auto& animation = entity.get_component<AnimationComponent>();
      auto& sprite = entity.get_component<SpriteComponent>();

      // Ensure frame_speed_rate is non-zero to avoid division by zero:
      if (animation.total_frames <= 1 || animation.frame_speed_rate == 0) {
        continue;
      }

      // Calculate frame based on elapsed VBlank frames divided by frame speed duration:
      u32 elapsed_frames = (current_ticks - animation.start_time) / animation.frame_speed_rate;

      // Update NFlib sprite hardware frame.
      // 3D sprites use NF_Set3dSpriteFrame, 2D sprites use NF_SpriteFrame.
      if (sprite.is_3D) {
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          u16 hw_total_frames = NF_3DSPRITE[sprite.id].lastframe + 1;
          u16 effective_frames = std::min(animation.total_frames, hw_total_frames);

          if (effective_frames <= 1) {
            continue;
          }

          if (animation.is_loop) {
            animation.current_frame = elapsed_frames % effective_frames;
          } else {
            animation.current_frame = (elapsed_frames >= effective_frames)
                                          ? (effective_frames - 1)
                                          : elapsed_frames;
          }

          NF_Set3dSpriteFrame(sprite.id, animation.current_frame);
        }
      } else {
        if (sprite.screen < 2 && sprite.id < 128 && NF_SPRITEOAM[sprite.screen][sprite.id].created) {
          if (animation.is_loop) {
            animation.current_frame = elapsed_frames % animation.total_frames;
          } else {
            animation.current_frame = (elapsed_frames >= animation.total_frames)
                                          ? (animation.total_frames - 1)
                                          : elapsed_frames;
          }

          NF_SpriteFrame(sprite.screen, sprite.id, animation.current_frame);
        }
      }
    }
  }
};

#endif  // ANIMATIONSYSTEM_H
