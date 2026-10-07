#include "../include/Game.h"

// Global variables:
u32 global_frame_counter = 0;
u16 keys = 0;
touchPosition touchscreen = {};

// Variables:
// TODO: move this to another place.
s16 x[MAXSPRITES];
s16 y[MAXSPRITES];
s16 ix = 4;
s16 iy = 4;

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

  // Retrieve slot IDs loaded by load_3d_sprite:
  u16 gfx_id = this->asset_manager->get_gfx_id("blueball");
  u16 pal_id = this->asset_manager->get_pal_id("blueball");

  // Instantiate background on Screen 0, Layer 3:
  NF_CreateTiledBg(0, 3, "bg3");

  // Enable alpha blending:
  REG_BLDCNT = BLEND_ALPHA
             | BLEND_SRC_BG0
             | BLEND_DST_BG1 | BLEND_DST_BG2 | BLEND_DST_BG3 | BLEND_DST_BACKDROP;

  // Initialize positions and instantiate 3D sprites in NFlib:
  for (size_t n = 0; n < MAXSPRITES; n++) {
    Entity entity = this->registry->create_entity();

    x[n] = 128 - 32;
    y[n] = 96 - 32;

    // 1. Tell NFlib to instantiate the 3D sprite hardware object
    // Signature: NF_Create3dSprite(sprite_slot, gfx_slot, pal_slot, x, y)
    NF_Create3dSprite(static_cast<u8>(n), gfx_id, pal_id, x[n], y[n]);

    // 2. Add transform component
    entity.add_component<TransformComponent>(Vec2f(x[n], y[n]));

    // 3. Add 3D Sprite component referencing real GFX and Palette IDs
    entity.add_component<SpriteComponent>(
      0,                     // Screen
      static_cast<u8>(n),    // 3D Sprite hardware slot ID
      gfx_id,                // Loaded GFX RAM/VRAM slot
      pal_id,                // Loaded Palette RAM/VRAM slot
      64,                    // Width
      64,                    // Height
      true,                  // is_3D
      true                   // is_rotscale
    );
  }

  // Sort priorities:
  NF_Sort3dSprites();

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
  this->asset_manager->load_tiled_bg("bg3", "bg/nature", 256, 256);
  this->asset_manager->load_3d_sprite("blueball", "sprite/blueball", 64, 64, 0, 0);
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
}

void Game::process_input(void) {
  // Scan hardware keys:
  scanKeys();
  uint32_t keys_pressed  = keysDown();
  uint32_t keys_released = keysUp();
  uint32_t keys_held     = keysHeld();

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

    // Emit click event using touch pixel coordinates (px, py)
    // this->event_manager->emit_event<ClickEvent>(KEY_TOUCH, touch.px, touch.py);
  }

  if (keys_released & KEY_TOUCH) {
    this->controller_manager->set_mouse_position(touch.px, touch.py);
    this->controller_manager->set_mouse_button_up(KEY_TOUCH);
  }
}

void Game::update(void) {
  // Increment global tick counter (used by AnimationSystem and timed events)
  global_frame_counter++;

  // Fixed delta time for 60Hz NDS hardware (~0.016667 seconds)
  // In fixed-point (20.12 format): ~68 units (1/60 * 4096)
  // If floating-point is needed:
  float delta_time = 1.0f / 60.0f;

  // Update the engine systems:
  this->event_manager->reset();
  this->registry->get_system<DamageSystem>().subscribe_to_collision_event(*this->event_manager);
  // this->registry->get_system<UISystem>().subscribe_to_click_event(*this->event_manager);  // TODO

  this->registry->update();

  // this->registry->get_system<ScriptSystem>().update(this->lua);  // TODO
  this->registry->get_system<AnimationSystem>().update();
  this->registry->get_system<CollisionSystem>().update(*this->event_manager);
  this->registry->get_system<MovementSystem>().update(delta_time);

  // Push updated sprite transformations to OAM VRAM:
  NF_SpriteOamSet(0);  // Top screen OAM update.
  NF_SpriteOamSet(1);  // Bottom screen OAM update.

  this->temporary();  // TODO: remove this from here.
}

// TODO: move this to another place.
void Game::temporary(void) {
    // Move tail sprites:
  for (int n = MAXSPRITES - 1; n > 0; n--) {
    x[n] = x[n - 1];
    y[n] = y[n - 1];

    NF_Blend3dSprite(n, n + 1, 31 - (n * 4));
    NF_Move3dSprite(n, x[n], y[n]);
  }

  // Move main sprite via touch or automatic velocity:
  if (keys & KEY_TOUCH) {
    x[0] = touchscreen.px - 32;
    y[0] = touchscreen.py - 32;

    if (x[0] < 8)   x[0] = 8;
    if (x[0] > 183) x[0] = 183;
    if (y[0] < 8)   y[0] = 8;
    if (y[0] > 119) y[0] = 119;
  }

  else {
    x[0] += ix;

    if (x[0] < 8) {
      x[0] = 8;
      ix = -ix;
    } else if (x[0] > 183) {
      x[0] = 183;
      ix = -ix;
    }

    y[0] += iy;

    if (y[0] < 8) {
      y[0] = 8;
      iy = -iy;
    } else if (y[0] > 119) {
      y[0] = 119;
      iy = -iy;
    }
  }

  NF_Move3dSprite(0, x[0], y[0]);
}

void Game::render(void) {
  // Update 2D sprite positions in NFlib OAM buffers via ECS:
  this->registry->get_system<RenderSystem>().update();

  // Update text positions / NFlib text layers:
  // TODO: include the asset manager as a parameter.
  // this->registry->get_system<RenderTextSystem>().update();  // TODO

  // Draw 3D sprites:
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
