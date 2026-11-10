#ifndef LEVELSCENE_H
#define LEVELSCENE_H

#include <nds.h>
#include <nf_lib.h>
#include <stdio.h>
#include <vector>

#include "Constants.h"
#include "Entity.h"
#include "Scene.h"

class LevelScene : public Scene {
 private:
  int player_lives = 1;
  bool is_paused = false;

  Entity player;
  const s16 player_speed = 2;

  // Menace enemy management:
  static constexpr size_t MAX_MENACES = 6;
  std::vector<u8> free_menace_slots;
  std::vector<Entity> active_menaces;
  u16 menace_spawn_timer = 0;
  u32 menaces_defeated_count = 0;

  // Bullet management:
  std::vector<u8> free_bullet_slots;
  std::vector<Entity> active_bullets;
  u8 shoot_cooldown = 0;

  // Background scrolling:
  s32 bg_buildings_scroll_x = 0;
  s32 bg_buildings_last_chunk = 0;
  static constexpr s32 BUILDINGS_TOTAL_WIDTH = 1024;
  static constexpr s32 BUILDINGS_CHUNK_WIDTH = 256;
  static constexpr s32 BUILDINGS_TOTAL_CHUNKS = BUILDINGS_TOTAL_WIDTH / BUILDINGS_CHUNK_WIDTH;
  s32 bg_front_beam_scroll_x = 0;
  const s32 front_beam_speed = 3;

  u32 last_update_tick = 0;

  void init_3D_sprites(void);
  void cleanup_entities(void);
  void spawn_player(void);
  void update_player_input(void);
  void shoot_bullet(void);
  void update_bullets(void);
  void spawn_menace(void);
  void update_menaces(void);
  void update_ui(void);

 public:
  LevelScene(void);
  ~LevelScene(void) override = default;

  void on_enter(void) override;
  void on_exit(void) override;
  void process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) override;
  void update(float delta_time) override;
  void render(void) override;
};

#endif  // LEVELSCENE_H

