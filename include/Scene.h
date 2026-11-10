#ifndef SCENE_H
#define SCENE_H

#include <nds.h>

class Game;

class Scene {
 public:
  virtual ~Scene(void) = default;

  virtual void on_enter(void) = 0;
  virtual void on_exit(void) = 0;
  virtual void process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) = 0;
  virtual void update(float delta_time) = 0;
  virtual void render(void) = 0;
};

#endif  // SCENE_H

