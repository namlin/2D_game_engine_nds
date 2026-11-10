#ifndef LEVELSCENE_H
#define LEVELSCENE_H

#include <nds.h>
#include <nf_lib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "Constants.h"
#include "Entity.h"
#include "LevelData.h"
#include "LevelLoader.h"
#include "Scene.h"

struct BgRuntimeState {
  BgLayerConfig config;
  s32 scroll_x = 0;
  s32 last_chunk = 0;
};

class LevelScene : public Scene {
 private:
  std::string level_file_path;
  LevelData level_data;

  int player_lives = 1;
  s16 player_speed = 2;
  bool is_paused = false;

  Entity player;

  // Level scroll progression (used to trigger furniture/prop and enemy spawns):
  s32 level_scroll_distance = 0;
  size_t next_prop_index = 0;
  size_t next_enemy_index = 0;

  // Enemy management (3D sprite slots 1..10):
  std::vector<u8> free_menace_slots;
  std::vector<Entity> active_menaces;
  u16 menace_spawn_timer = 0;
  u32 menaces_defeated_count = 0;

  // Bullet management (3D sprite slots 11..40):
  std::vector<u8> free_bullet_slots;
  std::vector<Entity> active_bullets;
  u8 shoot_cooldown = 0;

  // Furniture / Prop management (3D sprite slots 41..55):
  std::vector<u8> free_prop_slots;
  std::vector<Entity> active_props;

  // Background runtime scrolling states:
  std::vector<BgRuntimeState> bg_states;

  u32 last_update_tick = 0;

  void setup_default_level_data(void);
  void init_3D_sprites(void);
  void setup_level_assets_and_bgs(void);
  void cleanup_entities(void);
  void spawn_player(void);
  void update_player_input(void);
  void shoot_bullet(void);
  void update_bullets(void);
  void spawn_scripted_entities(void);
  void spawn_prop(const PropSpawnConfig& cfg);
  void update_props(void);
  void spawn_enemy_from_config(const EnemySpawnConfig& cfg);
  void spawn_random_menace(void);
  void update_menaces(void);
  void update_backgrounds(void);
  void update_ui(void);

 public:
  explicit LevelScene(const std::string& level_file_path = "levels/stage_1_1.lvl");
  ~LevelScene(void) override = default;

  void on_enter(void) override;
  void on_exit(void) override;
  void process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) override;
  void update(float delta_time) override;
  void render(void) override;
};

#endif  // LEVELSCENE_H
