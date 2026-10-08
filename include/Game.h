#ifndef GAME_H
#define GAME_H

#include <filesystem.h>
#include <nds.h>
#include <nf_lib.h>
#include <stdio.h>

#include <cstdint>

// Managers:
#include "../include/Constants.h"
#include "../include/AssetManager.h"
#include "../include/ControllerManager.h"
#include "../include/EventManager.h"

// ECS:
#include "../include/Component.h"
#include "../include/Entity.h"
#include "../include/Registry.h"
#include "../include/System.h"

// Components:
#include "../include/AnimationComponent.h"
#include "../include/CircleColliderComponent.h"
#include "../include/EnemyComponent.h"
#include "../include/HealthComponent.h"
#include "../include/ProjectileComponent.h"
#include "../include/RigidBodyComponent.h"
// #include "../include/ScriptComponent.h"
#include "../include/SpriteComponent.h"
#include "../include/TransformComponent.h"

// Systems:
#include "../include/AnimationSystem.h"
#include "../include/CollisionSystem.h"
#include "../include/DamageSystem.h"
#include "../include/MovementSystem.h"
#include "../include/RenderSystem.h"
// #include "../include/ScriptSystem.h"

#include "../include/ClickEvent.h"

class Game {
 private:
  // --- Attributes ---
  inline static Game* instance;

  uint8_t window_width = 255;
  uint8_t window_height = 192;

  const char* window_title = "2D Game Engine";

  uint16_t millisecs_previous_frame = 0;

  bool is_running = false;

  AssetManager* asset_manager = nullptr;
  EventManager* event_manager = nullptr;

  Registry* registry = nullptr;

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

  // Fixed delta time for 60Hz NDS hardware (~0.016667 seconds)
  // In fixed-point (20.12 format): ~68 units (1/60 * 4096)
  // If floating-point is needed:
  const float delta_time = 1.0f / 60.0f;

  // --- Singleton Encapsulation ---
  Game(void);
  ~Game(void);

  // Prevent copying:
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  // --- Private Methods ---
  void init_nitroFS(void);
  void init_assets(void);
  void init_3D_sprites(void);
  void update_player_input(void);
  void shoot_bullet(void);
  void update_bullets(void);
  void spawn_menace(void);
  void update_menaces(void);
  void update_ui(void);

  void process_input(void);
  void update(void);
  void render(void);

 public:
  ControllerManager* controller_manager = nullptr;

  static Game* get_instance(void);

  void init(void);
  void setup(void);
  void run(void);
  void destroy(void);
};

#endif  // GAME_H
