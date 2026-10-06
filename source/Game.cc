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
  // Initialize NitroFS first (this must happen before any file loads):
  this->init_nitroFS();

  // Set screen modes (Top screen = Mode 0 / 3D Engine, Bottom screen = Mode 0 / 2D Engine)
  NF_Set3D(0, 0);  // Display 3D engine on Top Screen (0).
  NF_Set2D(1, 0);  // Display 2D engine on Bottom Screen (1).

  // Setup text console on bottom screen without crashing display modes:
  consoleDemoInit();

  // Initialize OpenGL engine state for NFlib 3D Sprites:
  glInit();
  glViewport(0, 0, 255, 191);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrthof32(0, 256, 192, 0, -1024, 1024);  // Set 2D orthographic projection for 3D sprites.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Initialize tiled background system:
  NF_InitTiledBgBuffers();
  NF_InitTiledBgSys(0);  // Top screen.

  // Initialize 3D Sprite system and allocate slots;
  NF_InitSpriteBuffers();
  NF_Init3dSpriteSys();  // Allocate RAM structures for 3D sprites.

  // Load background files from NitroFS:
  NF_LoadTiledBg("bg/nature", "bg3", 256, 256);

  // Load sprite files from NitroFS:
  NF_LoadSpriteGfx("sprite/blueball", 0, 64, 64);
  NF_LoadSpritePal("sprite/blueball", 0);

  // Transfer sprites to VRAM:
  NF_Vram3dSpriteGfx(0, 0, true);
  NF_Vram3dSpritePal(0, 0);

  // Create background:
  NF_CreateTiledBg(0, 3, "bg3");

  // Enable alpha blending for 3D sprites over background layers
  REG_BLDCNT = BLEND_ALPHA
               | BLEND_SRC_BG0
               | BLEND_DST_BG1 | BLEND_DST_BG2 | BLEND_DST_BG3 | BLEND_DST_BACKDROP;

  // Initialize positions and create 3D sprites:
  for (size_t n = 0; n < MAXSPRITES; n++) {
    x[n] = 128 - 32;
    y[n] = 96 - 32;

    NF_Create3dSprite(n, 0, 0, x[n], y[n]);
  }

  // Sort priorities (lower IDs rendered on top):
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

  // Update input hardware state (libnds):
  scanKeys();

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

    if ((x[0] < 8) || (x[0] > 183)) ix = -ix;
      y[0] += iy;

    if ((y[0] < 8) || (y[0] > 119)) iy = -iy;
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
