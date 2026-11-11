#include "../include/TitleScene.h"
#include "../include/Game.h"

TitleScene::TitleScene(void) : last_title_tick(0), selected_stage(0) {}

void TitleScene::on_enter(void) {
  this->last_title_tick = 0;

  NF_HideBg(0, 1);
  NF_HideBg(0, 2);
  NF_HideBg(0, 3);

  AudioStreamer* audio = Game::get_instance()->get_audio_streamer();
  if (audio != nullptr) {
    audio->stop();
  }

  consoleClear();
}

void TitleScene::on_exit(void) {
  consoleClear();
}

void TitleScene::process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) {
  (void)keys_released;
  (void)keys_held;
  (void)touch;

  if (keys_pressed & (KEY_UP | KEY_LEFT)) {
    this->selected_stage = (this->selected_stage == 0) ? 3 : (this->selected_stage - 1);
    this->last_title_tick = 0;
  } else if (keys_pressed & (KEY_DOWN | KEY_RIGHT)) {
    this->selected_stage = (this->selected_stage + 1) % 4;
    this->last_title_tick = 0;
  }

  if ((keys_pressed & (KEY_START | KEY_A)) || (keys_pressed & KEY_TOUCH)) {
    if (this->selected_stage == 0) {
      Game::get_instance()->get_scene_manager()->switch_to("level_1");
    } else if (this->selected_stage == 1) {
      Game::get_instance()->get_scene_manager()->switch_to("level_2");
    } else if (this->selected_stage == 2) {
      Game::get_instance()->get_scene_manager()->switch_to("free_roam");
    } else {
      Game::get_instance()->get_scene_manager()->switch_to("castlevania");
    }
  }
}

void TitleScene::update(float delta_time) {
  (void)delta_time;
  this->update_title_ui();
}

void TitleScene::render(void) {}

void TitleScene::update_title_ui(void) {
  if (this->last_title_tick != 0 && (global_frame_counter - this->last_title_tick < 10)) {
    return;
  }

  this->last_title_tick = global_frame_counter;

  printf("\x1b[2;2H==============================");
  printf("\x1b[3;2H     2D GAME ENGINE (NDS)     ");
  printf("\x1b[4;2H==============================");

  printf("\x1b[6;3HSelect Stage (D-Pad):");
  printf("\x1b[7;4H%s Stage 1-1: City Assault  ", (this->selected_stage == 0) ? ">" : " ");
  printf("\x1b[8;4H%s Stage 1-2: Nature Sector ", (this->selected_stage == 1) ? ">" : " ");
  printf("\x1b[9;4H%s Free Move: 768x512 Arena ", (this->selected_stage == 2) ? ">" : " ");
  printf("\x1b[10;4H%s Castlevania: Hugh's Hall", (this->selected_stage == 3) ? ">" : " ");

  if ((global_frame_counter / 30) % 2 == 0) {
    printf("\x1b[12;5H>>> PRESS START OR A <<<    ");
    printf("\x1b[13;6HTOUCH SCREEN TO PLAY       ");
  } else {
    printf("\x1b[12;5H                            ");
    printf("\x1b[13;6H                            ");
  }

  printf("\x1b[15;2HControls:");
  printf("\x1b[16;4H- D-Pad : Move / Menu");
  printf("\x1b[17;4H- B / A : Jump (Castlevania)");
  printf("\x1b[18;4H- Y     : Shoot / Attack");
  printf("\x1b[19;4H- START : Start / Pause");
  printf("\x1b[20;4H- SELECT: Shutdown Console");
  printf("\x1b[22;2H==============================");
}
