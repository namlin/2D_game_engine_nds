#include "../include/Game.h"

// Global variables:
u32 global_frame_counter = 0;
u16 keys = 0;
touchPosition touchscreen = {};


Game::Game(void) {
  this->asset_manager = new AssetManager();

  if (this->asset_manager == nullptr) {
    NF_Error(1, "[GAME] ERROR: No dynamic memory was allocated for asset_manager.", 1);
    return;
  }

  this->controller_manager = new ControllerManager();

  if (this->controller_manager == nullptr) {
    NF_Error(1, "[Game] ERROR: No dynamic memory was allocated for controller_manager.", 1);
    return;
  }

  this->event_manager = new EventManager();

  if (this->event_manager == nullptr) {
    NF_Error(1, "[Game] ERROR: No dynamic memory was allocated for event_manager.", 1);
    return;
  }

  // TODO: SceneManager

  this->registry = new Registry();

  if (this->registry == nullptr) {
    NF_Error(1, "[Game] ERROR: No dynamic memory was allocated for registry.", 1);
    return;
  }
}

Game::~Game(void) {
  delete this->asset_manager;
  delete this->controller_manager;
  delete this->event_manager;
  // delete this->scene_manager;  // TODO
  delete this->registry;

  this->asset_manager = nullptr;
  this->controller_manager = nullptr;
  this->event_manager = nullptr;
  // this->scene_manager = nullptr;  // TODO
  this->registry = nullptr;
}

Game* Game::get_instance(void) {
  if (!instance) {
    instance = new Game();

    if (instance == nullptr) {
      NF_Error(1, "No dynamic memory was allocated for the Game instance.", 1);
    }
  }

  return instance;
}

void Game::init(void) {
  // Initialize NitroFS first:
  this->init_nitroFS();

  // Set screen modes:
  NF_Set3D(0, 0);  // Display 3D engine on Top Screen (0).
  NF_Set2D(1, 0);  // Display 2D engine on Bottom Screen (1).

  // Setup text console:
  consoleDemoInit();

  // Initialize OpenGL engine state for NFlib 3D Sprites:
  glInit();
  glViewport(0, 0, 255, 191);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrthof32(0, 256, 192, 0, -1024, 1024);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Initialize tiled background system:
  NF_InitTiledBgBuffers();
  NF_InitTiledBgSys(0);

  // Initialize 3D Sprite system:
  NF_InitSpriteBuffers();
  NF_Init3dSpriteSys();

  // Load assets into RAM/VRAM via AssetManager:
  this->init_assets();

  this->is_running = true;
}

void Game::init_nitroFS(void) {
  // Initialize NitroFS:
  if (!nitroFSInit(NULL)) {
    while (1) {
      swiWaitForVBlank();  // Spin loop on failure.
    }
  }

  NF_SetRootFolder("NITROFS");
}

void Game::init_assets(void) {
  // (Loads files from NitroFS into RAM and transfers graphics/palettes to VRAM):

  // Backgrounds:
  this->asset_manager->load_tiled_bg("bg3", "bg/nature", 256, 256);

  // Instantiate background on Screen 0, Layer 3:
  NF_CreateTiledBg(0, 3, "bg3");

  // Sprites:
  this->asset_manager->load_3d_sprite("Player_1", "sprite/Player_1", 64, 64, 0, 0, false);
  this->asset_manager->load_3d_sprite("Menace", "sprite/Menace", 32, 32, 1, 1, false);
  this->asset_manager->load_3d_sprite("Player_Bullet", "sprite/Player_Bullet", BULLET_WIDTH, BULLET_HEIGHT, 2, 2, false);

  // sets up a transparency blend:
  // Enable alpha blending:
  REG_BLDCNT = BLEND_ALPHA
             | BLEND_SRC_BG0
             | BLEND_DST_BG1 | BLEND_DST_BG2 | BLEND_DST_BG3 | BLEND_DST_BACKDROP;

  this->init_3D_sprites();
}

// Initialize positions and instantiate 3D sprites in NFlib:
void Game::init_3D_sprites(void) {
  srand(0x1337);

  // 1. Player_1 (Slot 0):
  u16 player_gfx_id = this->asset_manager->get_gfx_id("Player_1");
  u16 player_pal_id = this->asset_manager->get_pal_id("Player_1");

  s16 player_start_x = (SCREEN_WIDTH - PLAYER_WIDTH) / 2;
  s16 player_start_y = (SCREEN_HEIGHT - PLAYER_HEIGHT) / 2;

  // Tell NFlib to instantiate the 3D sprite hardware object:
  NF_Create3dSprite(0, player_gfx_id, player_pal_id, player_start_x, player_start_y);

  this->player = this->registry->create_entity();
  this->player.add_component<TransformComponent>(Vec2f(player_start_x, player_start_y));
  this->player.add_component<SpriteComponent>(
    0,               // Screen (0 = Top Screen)
    0,               // 3D Sprite hardware slot ID
    player_gfx_id,   // Loaded GFX RAM/VRAM slot
    player_pal_id,   // Loaded Palette RAM/VRAM slot
    PLAYER_WIDTH,    // Width (64)
    PLAYER_HEIGHT,   // Height (64)
    true,            // is_3D
    false            // is_rotscale
  );

  // Initialize pool of available 3D sprite slots for Menaces (slots 1 to 10):
  this->free_menace_slots.clear();
  for (u8 slot = 1; slot <= 10; slot++) {
    this->free_menace_slots.push_back(slot);
  }
  this->active_menaces.clear();
  this->menace_spawn_timer = 20; // Spawn first enemy shortly after start
  this->menaces_defeated_count = 0;

  // Initialize pool of available 3D sprite slots for bullets (slots 11 to 40):
  this->free_bullet_slots.clear();
  for (u8 slot = 11; slot <= 40; slot++) {
    this->free_bullet_slots.push_back(slot);
  }
  this->active_bullets.clear();

  NF_Sort3dSprites();  // Sort priorities.
}

void Game::setup(void) {
  // Add systems:
  this->registry->add_system<AnimationSystem>();
  this->registry->add_system<CollisionSystem>();
  this->registry->add_system<DamageSystem>();
  this->registry->add_system<MovementSystem>();
  this->registry->add_system<RenderSystem>();
  // this->registry->add_system<RenderTextSystem>();  // TODO
  // this->registry->add_system<ScriptSystem>();  // TODO
  // this->registry->add_system<UISystem>();  // TODO

  // this->scene_manager->load_scene_from_script("./assets/lua_scripts/scenes.lua", this->lua);  // TODO

  // this->lua.open_libraries(sol::lib::base, sol::lib::math);  // TODO
  // this->registry->get_system<ScriptSystem>().create_lua_binding(this->lua);  // TODO

  // Map keys:
  this->controller_manager->add_action_key("Move Up", KEY_UP);
  this->controller_manager->add_action_key("Move Down", KEY_DOWN);
  this->controller_manager->add_action_key("Move Left", KEY_LEFT);
  this->controller_manager->add_action_key("Move Right", KEY_RIGHT);
  this->controller_manager->add_action_key("Shoot", KEY_A | KEY_B | KEY_Y);
}

void Game::process_input(void) {
  // Scan hardware keys:
  scanKeys();
  uint32_t keys_pressed = keysDown();
  uint32_t keys_released = keysUp();
  uint32_t keys_held = keysHeld();

  // Read raw touch coordinates directly from libnds:
  touchPosition touch;
  touchRead(&touch);

  keys = static_cast<u16>(keys_held);
  touchscreen = touch;

  // Handle Quit / Exit triggers:
  if (keys_pressed & KEY_START) {
    // this->scene_manager->stop_scene();
    this->is_running = false;
    return;
  }

  // Update ControllerManager's key states:
  this->controller_manager->update();

  // Touch Screen / Mouse Events:
  if (keys_held & KEY_TOUCH) {
    this->controller_manager->set_mouse_position(touch.px, touch.py);
  }

  if (keys_pressed & KEY_TOUCH) {
    this->controller_manager->set_mouse_position(touch.px, touch.py);
    this->controller_manager->set_mouse_button_down(KEY_TOUCH);

    // Emit click event using touch pixel coordinates (px, py):
    // this->event_manager->emit_event<ClickEvent>(KEY_TOUCH, touch.px, touch.py);
  }

  if (keys_released & KEY_TOUCH) {
    this->controller_manager->set_mouse_position(touch.px, touch.py);
    this->controller_manager->set_mouse_button_up(KEY_TOUCH);
  }
}

void Game::update_player_input(void) {
  if (!this->player.has_component<TransformComponent>()) {
    return;
  }

  auto& transform = this->player.get_component<TransformComponent>();

  // Apply movement based on D-pad input via ControllerManager and hardware keys:
  if (this->controller_manager->is_action_activated("Move Left") || (keys & KEY_LEFT)) {
    transform.position.x -= this->player_speed;
  }
  if (this->controller_manager->is_action_activated("Move Right") || (keys & KEY_RIGHT)) {
    transform.position.x += this->player_speed;
  }
  if (this->controller_manager->is_action_activated("Move Up") || (keys & KEY_UP)) {
    transform.position.y -= this->player_speed;
  }
  if (this->controller_manager->is_action_activated("Move Down") || (keys & KEY_DOWN)) {
    transform.position.y += this->player_speed;
  }

  // Prevent sprite from moving outside screen limits (screen: 256x192, sprite: 64x64):
  if (transform.position.x < 0) {
    transform.position.x = 0;
  } else if (transform.position.x > static_cast<s32>(SCREEN_WIDTH - PLAYER_WIDTH)) {
    transform.position.x = static_cast<s32>(SCREEN_WIDTH - PLAYER_WIDTH);
  }

  if (transform.position.y < 0) {
    transform.position.y = 0;
  } else if (transform.position.y > static_cast<s32>(SCREEN_HEIGHT - PLAYER_HEIGHT)) {
    transform.position.y = static_cast<s32>(SCREEN_HEIGHT - PLAYER_HEIGHT);
  }

  // Handle shooting:
  if (this->shoot_cooldown > 0) {
    this->shoot_cooldown--;
  }

  if (this->controller_manager->is_action_activated("Shoot") || (keys & (KEY_A | KEY_B | KEY_Y))) {
    if (this->shoot_cooldown == 0) {
      this->shoot_bullet();
      this->shoot_cooldown = 12; // Rate limit between shots (~5 shots/sec)
    }
  }
}

void Game::shoot_bullet(void) {
  if (this->free_bullet_slots.empty()) {
    return;
  }

  if (!this->player.has_component<TransformComponent>()) {
    return;
  }

  u8 slot = this->free_bullet_slots.back();
  this->free_bullet_slots.pop_back();

  const auto& player_transform = this->player.get_component<TransformComponent>();

  u16 bullet_gfx_id = this->asset_manager->get_gfx_id("Player_Bullet");
  u16 bullet_pal_id = this->asset_manager->get_pal_id("Player_Bullet");

  // Spawn bullet in front of the player ship:
  s16 bullet_start_x = player_transform.position.x + PLAYER_WIDTH - 8;
  s16 bullet_start_y = player_transform.position.y + (PLAYER_HEIGHT / 2) - (BULLET_HEIGHT / 2);

  NF_Create3dSprite(slot, bullet_gfx_id, bullet_pal_id, bullet_start_x, bullet_start_y);

  Entity bullet = this->registry->create_entity();
  bullet.add_component<TransformComponent>(Vec2f(bullet_start_x, bullet_start_y));
  bullet.add_component<RigidBodyComponent>(Vec2f(240, 0)); // 240 px/sec (4 pixels per frame)
  bullet.add_component<SpriteComponent>(
    0,               // Screen (0 = Top Screen)
    slot,            // 3D Sprite hardware slot ID
    bullet_gfx_id,   // Loaded GFX RAM/VRAM slot
    bullet_pal_id,   // Loaded Palette RAM/VRAM slot
    BULLET_WIDTH,    // Width (8)
    BULLET_HEIGHT,   // Height (8)
    true,            // is_3D
    false            // is_rotscale
  );
  bullet.add_component<AnimationComponent>(
    2,               // 2 frames
    8,               // Switch frame every 8 frames
    true             // Loop animation
  );
  bullet.add_component<CircleColliderComponent>(4, BULLET_WIDTH, BULLET_HEIGHT);
  bullet.add_component<ProjectileComponent>(20, true);

  this->active_bullets.push_back(bullet);
}

void Game::update_bullets(void) {
  for (auto it = this->active_bullets.begin(); it != this->active_bullets.end(); ) {
    Entity bullet = *it;

    // Check if bullet was destroyed by DamageSystem upon collision:
    if (this->registry->is_entity_to_remove(bullet) || !bullet.has_component<TransformComponent>()) {
      if (bullet.has_component<SpriteComponent>()) {
        auto& sprite = bullet.get_component<SpriteComponent>();
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          NF_Delete3dSprite(sprite.id);
        }
        this->free_bullet_slots.push_back(sprite.id);
      }
      it = this->active_bullets.erase(it);
      continue;
    }

    auto& transform = bullet.get_component<TransformComponent>();

    // Check if bullet traveled off screen:
    if (transform.position.x > static_cast<s32>(SCREEN_WIDTH) || transform.position.x < -16 ||
        transform.position.y > static_cast<s32>(SCREEN_HEIGHT) || transform.position.y < -16) {
      if (bullet.has_component<SpriteComponent>()) {
        auto& sprite = bullet.get_component<SpriteComponent>();
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          NF_Delete3dSprite(sprite.id);
        }
        this->free_bullet_slots.push_back(sprite.id);
      }
      bullet.delete_entity();
      it = this->active_bullets.erase(it);
      continue;
    }

    ++it;
  }
}

void Game::spawn_menace(void) {
  if (this->free_menace_slots.empty() || this->active_menaces.size() >= MAX_MENACES) {
    return;
  }

  u8 slot = this->free_menace_slots.back();
  this->free_menace_slots.pop_back();

  u16 menace_gfx_id = this->asset_manager->get_gfx_id("Menace");
  u16 menace_pal_id = this->asset_manager->get_pal_id("Menace");

  // Spawn from the right edge of the screen at random height:
  s16 spawn_x = SCREEN_WIDTH;
  s16 max_y = SCREEN_HEIGHT - MENACE_HEIGHT;
  s16 spawn_y = rand() % (max_y + 1);

  // Random speed between 35 and 75 px/s:
  s32 speed = 35 + (rand() % 41);

  NF_Create3dSprite(slot, menace_gfx_id, menace_pal_id, spawn_x, spawn_y);

  Entity new_menace = this->registry->create_entity();
  new_menace.add_component<TransformComponent>(Vec2f(spawn_x, spawn_y));
  new_menace.add_component<RigidBodyComponent>(Vec2f(-speed, 0));
  new_menace.add_component<SpriteComponent>(
    0,               // Screen (0 = Top Screen)
    slot,            // 3D Sprite hardware slot ID
    menace_gfx_id,   // Loaded GFX RAM/VRAM slot
    menace_pal_id,   // Loaded Palette RAM/VRAM slot
    MENACE_WIDTH,    // Width (32)
    MENACE_HEIGHT,   // Height (32)
    true,            // is_3D
    false            // is_rotscale
  );
  new_menace.add_component<AnimationComponent>(
    4,               // 4 frames (each 32x32)
    30,              // Animation speed: switch frame every 30 frames
    true             // Loop animation
  );
  new_menace.add_component<CircleColliderComponent>(16, MENACE_WIDTH, MENACE_HEIGHT);
  new_menace.add_component<HealthComponent>(40, 40);
  new_menace.add_component<EnemyComponent>(speed);

  this->active_menaces.push_back(new_menace);
}

void Game::update_menaces(void) {
  // Spawn countdown:
  if (this->menace_spawn_timer > 0) {
    this->menace_spawn_timer--;
  } else {
    this->spawn_menace();
    this->menace_spawn_timer = 45 + (rand() % 45); // Spawn next Menace in ~0.75-1.5 seconds
  }

  // Get player center position for homing/tracking:
  Vec2f player_center(0, 0);
  bool has_player = this->player.has_component<TransformComponent>();
  if (has_player) {
    const auto& pt = this->player.get_component<TransformComponent>();
    player_center.x = pt.position.x + (PLAYER_WIDTH / 2);
    player_center.y = pt.position.y + (PLAYER_HEIGHT / 2);
  }

  for (auto it = this->active_menaces.begin(); it != this->active_menaces.end(); ) {
    Entity menace_entity = *it;

    // Check if Menace was destroyed by DamageSystem upon collision:
    if (this->registry->is_entity_to_remove(menace_entity) || !menace_entity.has_component<TransformComponent>()) {
      if (menace_entity.has_component<SpriteComponent>()) {
        auto& sprite = menace_entity.get_component<SpriteComponent>();
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          NF_Delete3dSprite(sprite.id);
        }
        this->free_menace_slots.push_back(sprite.id);
      }
      this->menaces_defeated_count++;
      it = this->active_menaces.erase(it);
      continue;
    }

    auto& transform = menace_entity.get_component<TransformComponent>();

    // Check if Menace traveled off-screen to the left:
    if (transform.position.x < -static_cast<s32>(MENACE_WIDTH) ||
        transform.position.y < -32 || transform.position.y > static_cast<s32>(SCREEN_HEIGHT + 32)) {
      if (menace_entity.has_component<SpriteComponent>()) {
        auto& sprite = menace_entity.get_component<SpriteComponent>();
        if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
          NF_Delete3dSprite(sprite.id);
        }
        this->free_menace_slots.push_back(sprite.id);
      }
      menace_entity.delete_entity();
      it = this->active_menaces.erase(it);
      continue;
    }

    // Move to the player position at the enemy's individual speed:
    if (has_player && menace_entity.has_component<RigidBodyComponent>() && menace_entity.has_component<EnemyComponent>()) {
      auto& rb = menace_entity.get_component<RigidBodyComponent>();
      const auto& enemy = menace_entity.get_component<EnemyComponent>();

      s32 menace_cx = transform.position.x + (MENACE_WIDTH / 2);
      s32 menace_cy = transform.position.y + (MENACE_HEIGHT / 2);

      s32 dx = player_center.x - menace_cx;
      s32 dy = player_center.y - menace_cy;

      u32 dist_sq = (dx * dx) + (dy * dy);
      u32 dist = sqrt32(dist_sq);

      if (dist > 0) {
        rb.velocity.x = (dx * enemy.speed) / static_cast<s32>(dist);
        rb.velocity.y = (dy * enemy.speed) / static_cast<s32>(dist);
      }
    }

    ++it;
  }
}

void Game::update_ui(void) {
  static u32 last_update_tick = 0;
  if (global_frame_counter - last_update_tick < 5) {
    return;
  }
  last_update_tick = global_frame_counter;

  printf("\x1b[1;2H================================");
  printf("\x1b[2;2H     2D GAME ENGINE (NDS)       ");
  printf("\x1b[3;2H================================");
  printf("\x1b[5;2HControls:");
  printf("\x1b[6;4H- D-Pad : Move Player");
  printf("\x1b[7;4H- A/B/Y : Shoot Bullet");
  printf("\x1b[9;2H--------------------------------");
  printf("\x1b[10;2HActive Menaces : %2d / %2d      ", static_cast<int>(this->active_menaces.size()), static_cast<int>(MAX_MENACES));
  printf("\x1b[11;2HDefeated Score : %4lu           ", this->menaces_defeated_count);
  printf("\x1b[12;2HActive Bullets : %2d            ", static_cast<int>(this->active_bullets.size()));
  printf("\x1b[14;2HStatus: %s", this->active_menaces.empty() ? "SCANNING AREA... " : "SWARM INCOMING! ");
}

void Game::update(void) {
  // Increment global tick counter (used by AnimationSystem and timed events)
  global_frame_counter++;

  // Update the engine systems:
  this->event_manager->reset();
  this->registry->get_system<DamageSystem>().subscribe_to_collision_event(*this->event_manager);
  // this->registry->get_system<UISystem>().subscribe_to_click_event(*this->event_manager);  // TODO

  this->registry->update();

  // Handle player movement and boundary limits (including shooting):
  this->update_player_input();

  // this->registry->get_system<ScriptSystem>().update(this->lua);  // TODO
  this->registry->get_system<AnimationSystem>().update();
  this->registry->get_system<CollisionSystem>().update(*this->event_manager);
  this->registry->get_system<MovementSystem>().update(this->delta_time);

  // Update enemy spawns and movement towards player:
  this->update_menaces();

  // Update bullet states and release off-screen/collided sprites:
  this->update_bullets();

  // Update bottom screen stats:
  this->update_ui();

  // Push updated sprite transformations to OAM VRAM:
  NF_SpriteOamSet(0);  // Top screen OAM update.
  NF_SpriteOamSet(1);  // Bottom screen OAM update.
}

void Game::render(void) {
  // Update 2D sprite positions in NFlib OAM buffers via ECS:
  this->registry->get_system<RenderSystem>().update();

  // Update text positions / NFlib text layers:
  // TODO: include the asset manager as a parameter.
  // this->registry->get_system<RenderTextSystem>().update();  // TODO

  NF_Draw3dSprites();
  glFlush(0);

  // Flush NFlib's shadow OAM buffer to the DS hardware OAM registers.
  // Screen 0 = Top Display, Screen 1 = Bottom Display.
  NF_SpriteOamSet(0);
  NF_SpriteOamSet(1);

  oamUpdate(&oamMain);
  oamUpdate(&oamSub);

  // Synchronize to frame refresh (~60 FPS VBlank interrupt):
  swiWaitForVBlank();

  // Update animated 3D sprite graphics textures if needed:
  NF_Update3dSpritesGfx();
}

void Game::run(void) {
  while (this->is_running) {
    this->process_input();
    this->update();
    this->render();
  }
}

void Game::destroy(void) {
  if (this->asset_manager != nullptr) {
    this->asset_manager->clear_assets();
  }

  delete instance;
  instance = nullptr;
}
