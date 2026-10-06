#ifndef ANIMATIONCOMPONENT_H
#define ANIMATIONCOMPONENT_H

#include <nds.h>
#include <nf_lib.h>

struct AnimationComponent {
  u16 total_frames = 0;
  u16 current_frame = 0;
  u16 frame_speed_rate = 0;
  u32 start_time = 0;
  bool is_loop = false;

  AnimationComponent(u16 total_frames = 1, u16 frame_speed_rate = 1,
                     bool is_loop = true) {
    this->total_frames = total_frames;
    this->current_frame = 0;
    this->frame_speed_rate = frame_speed_rate;
    this->is_loop = is_loop;
    this->start_time = NF_GetFrame();
  }
};

#endif // ANIMATIONCOMPONENT_H
