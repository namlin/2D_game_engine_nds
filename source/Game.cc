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

  // sets up a transparency blend:
  // Enable alpha blending:
  REG_BLDCNT = BLEND_ALPHA
             | BLEND_SRC_BG0
             | BLEND_DST_BG1 | BLEND_DST_BG2 | BLEND_DST_BG3 | BLEND_DST_BACKDROP;

  this->init_3D_sprites();
}

// Initialize positions and instantiate 3D sprites in NFlib:
void Game::init_3D_sprites(void) {
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

  // 2. Menace (Slot 1) positioned on the right side of the screen:
  u16 menace_gfx_id = this->asset_manager->get_gfx_id("Menace");
  u16 menace_pal_id = this->asset_manager->get_pal_id("Menace");

  s16 menace_start_x = SCREEN_WIDTH - MENACE_WIDTH;          // 256 - 32 = 224 (right edge)
  s16 menace_start_y = (SCREEN_HEIGHT - MENACE_HEIGHT) / 2;  // 80 (centered vertically)

  NF_Create3dSprite(1, menace_gfx_id, menace_pal_id, menace_start_x, menace_start_y);

  this->menace = this->registry->create_entity();
  this->menace.add_component<TransformComponent>(Vec2f(menace_start_x, menace_start_y));
  this->menace.add_component<SpriteComponent>(
    0,               // Screen (0 = Top Screen)
    1,               // 3D Sprite hardware slot ID
    menace_gfx_id,   // Loaded GFX RAM/VRAM slot
    menace_pal_id,   // Loaded Palette RAM/VRAM slot
    MENACE_WIDTH,    // Width (32)
    MENACE_HEIGHT,   // Height (32)
    true,            // is_3D
    false            // is_rotscale
  );
  this->menace.add_component<AnimationComponent>(
    4,               // 4 frames (each 32x32)
    8,               // Animation speed: switch frame every 8 frames
    true             // Loop animation
  );

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
}

void Game::update(void) {
  // Increment global tick counter (used by AnimationSystem and timed events)
  global_frame_counter++;

  // Update the engine systems:
  this->event_manager->reset();
  this->registry->get_system<DamageSystem>().subscribe_to_collision_event(*this->event_manager);
  // this->registry->get_system<UISystem>().subscribe_to_click_event(*this->event_manager);  // TODO

  this->registry->update();

  // Handle player movement and boundary limits:
  this->update_player_input();

  // this->registry->get_system<ScriptSystem>().update(this->lua);  // TODO
  this->registry->get_system<AnimationSystem>().update();
  this->registry->get_system<CollisionSystem>().update(*this->event_manager);
  this->registry->get_system<MovementSystem>().update(this->delta_time);

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
