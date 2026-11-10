#ifndef GAME_H
#define GAME_H

#include <filesystem.h>
#include <nds.h>
#include <nf_lib.h>

#include <stdio.h>

#include <cstdint>

#include "../include/Constants.h"

// Managers:
#include "../include/AssetManager.h"
#include "../include/AudioStreamer.h"
#include "../include/ControllerManager.h"
#include "../include/EventManager.h"
#include "../include/SceneManager.h"

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

// Events:
#include "../include/ClickEvent.h"

// Scenes:
#include "../include/LevelScene.h"
#include "../include/TitleScene.h"

class Game {
 private:
  // --- Attributes ---
  inline static Game* instance;

  uint8_t window_width = 255;
  uint8_t window_height = 192;

  const char* window_title = "2D Game Engine";

  uint16_t millisecs_previous_frame = 0;

  bool is_running = false;

  AudioStreamer* audio_streamer = nullptr;

  AssetManager* asset_manager = nullptr;
  EventManager* event_manager = nullptr;
  SceneManager* scene_manager = nullptr;

  Registry* registry = nullptr;

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

  void process_input(void);
  void update(void);
  void render(void);

 public:
  ControllerManager* controller_manager = nullptr;

  static Game* get_instance(void);

  AssetManager* get_asset_manager(void) const { return this->asset_manager; }
  EventManager* get_event_manager(void) const { return this->event_manager; }
  AudioStreamer* get_audio_streamer(void) const { return this->audio_streamer; }
  SceneManager* get_scene_manager(void) const { return this->scene_manager; }
  Registry* get_registry(void) const { return this->registry; }

  void init(void);
  void setup(void);
  void run(void);
  void destroy(void);
};

#endif  // GAME_H
