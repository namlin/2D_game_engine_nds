#ifndef TITLESCENE_H
#define TITLESCENE_H

#include <nds.h>
#include <nf_lib.h>
#include <stdio.h>

#include "Constants.h"
#include "Scene.h"

class TitleScene : public Scene {
 private:
  u32 last_title_tick = 0;
  u8 selected_stage = 0;

  void update_title_ui(void);

 public:
  TitleScene(void);
  ~TitleScene(void) override = default;

  void on_enter(void) override;
  void on_exit(void) override;
  void process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) override;
  void update(float delta_time) override;
  void render(void) override;
};

#endif  // TITLESCENE_H
