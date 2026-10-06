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
// #include "../include/EventManager.h"

// ECS:
#include "../include/Component.h"
#include "../include/Entity.h"
#include "../include/Registry.h"
#include "../include/System.h"

// Components:
#include "../include/AnimationComponent.h"
#include "../include/CircleColliderComponent.h"
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

  // --- Singleton Encapsulation ---
  Game(void);
  ~Game(void);

  // Prevent copying:
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;

  // --- Private Methods ---
  void init_nitroFS(void);
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
