#ifndef LEVELDATA_H
#define LEVELDATA_H

#include <nds.h>
#include <string>
#include <vector>

struct BgmConfig {
  std::string file_path = "";
  u32 sample_rate = 22050;
  bool loop = true;
};

struct BgLayerConfig {
  u8 layer = 3;
  std::string bg_id = "";
  std::string file_path = "";
  u16 width = 256;
  u16 height = 256;
  u8 priority = 3;
  s32 scroll_speed_x = 0;
  s32 scroll_speed_y = 0;
  s32 stream_total_width = 0;  // 0 = standard hardware scroll, >0 = circular DMA chunk streaming
};

struct SpriteAssetConfig {
  std::string sprite_id = "";
  std::string file_path = "";
  u16 width = 32;
  u16 height = 32;
  u16 gfx_slot = 0;
  u16 pal_slot = 0;
  bool keepframes = false;
};

struct PlayerConfig {
  std::string sprite_id = "Player_1";
  s16 start_x = 20;
  s16 start_y = 64;
  s16 speed = 2;
  int lives = 1;
  u16 width = 64;
  u16 height = 64;
  u8 num_frames = 2;
  u16 collider_radius = 16;
};

struct PropSpawnConfig {
  s32 trigger_distance = 0;
  std::string sprite_id = "";
  s16 x = 0;
  s16 y = 0;
  u16 width = 32;
  u16 height = 32;
  u16 collider_radius = 16;
  s32 velocity_x = -60;
  s32 velocity_y = 0;
  int hp = 0;  // 0 = indestructible furniture/obstacle, >0 = destructible prop
};

struct PlatformConfig {
  std::string sprite_id = "Platform";
  s16 x = 0;
  s16 y = 0;
  u16 width = 32;
  u16 height = 16;
};

struct EnemySpawnConfig {
  s32 trigger_distance = 0;
  std::string sprite_id = "Menace";
  s16 x = 256;
  s16 y = 80;
  u16 width = 32;
  u16 height = 32;
  u8 num_frames = 4;
  u8 anim_speed = 30;
  u16 collider_radius = 16;
  s32 speed = 50;
  int hp = 40;
  bool homing = true;
};

struct RandomSpawnerConfig {
  bool enabled = true;
  std::string sprite_id = "Menace";
  u16 min_interval = 45;
  u16 max_interval = 90;
  u8 max_active = 6;
  s32 min_speed = 35;
  s32 max_speed = 75;
  int hp = 40;
};

struct LevelData {
  std::string name = "STAGE 1-1";
  std::string next_scene = "title";
  u32 target_score = 10;
  u8 sprite_layer_priority = 1;

  // World bounds & camera mode:
  bool free_roam = false;
  s32 world_width = 256;
  s32 world_height = 192;

  // Platformer & gravity mode (e.g. Castlevania level):
  bool platformer = false;
  s32 gravity = 12;          // Vertical acceleration in 1/64 px per frame^2
  s32 jump_velocity = -270;  // Jump impulse in 1/64 px per frame
  s32 ground_y = 176;        // Solid floor Y coordinate in world space

  BgmConfig bgm;
  PlayerConfig player;
  RandomSpawnerConfig spawner;

  std::vector<BgLayerConfig> backgrounds;
  std::vector<SpriteAssetConfig> sprite_assets;
  std::vector<PropSpawnConfig> props;
  std::vector<PlatformConfig> platforms;
  std::vector<EnemySpawnConfig> enemies;
};

#endif  // LEVELDATA_H
