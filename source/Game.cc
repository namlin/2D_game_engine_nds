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

  this->scene_manager = new SceneManager();

  if (this->scene_manager == nullptr) {
    NF_Error(1, "[Game] ERROR: No dynamic memory was allocated for scene_manager.", 1);
    return;
  }

  this->registry = new Registry();

  if (this->registry == nullptr) {
    NF_Error(1, "[Game] ERROR: No dynamic memory was allocated for registry.", 1);
    return;
  }
}

Game::~Game(void) {
  delete this->scene_manager;
  delete this->asset_manager;
  delete this->controller_manager;
  delete this->event_manager;
  delete this->registry;

  if (this->audio_streamer != nullptr) {
    delete this->audio_streamer;
    this->audio_streamer = nullptr;
  }

  this->scene_manager = nullptr;
  this->asset_manager = nullptr;
  this->controller_manager = nullptr;
  this->event_manager = nullptr;
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

  // Initialize sound hardware, NFlib raw sound buffers, and audio streamer:
  soundEnable();
  NF_InitRawSoundBuffers();
  this->audio_streamer = new AudioStreamer();
  this->audio_streamer->init();

  // Configure hardware blending and global asset defaults:
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
  // Set up a transparency blend:
  // Enable alpha blending between 3D sprites (BG0) and tiled background layers (BG1..BG3):
  REG_BLDCNT = BLEND_ALPHA
             | BLEND_SRC_BG0
             | BLEND_DST_BG1 | BLEND_DST_BG2 | BLEND_DST_BG3 | BLEND_DST_BACKDROP;
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

  // Map keys:
  this->controller_manager->add_action_key("Move Up", KEY_UP);
  this->controller_manager->add_action_key("Move Down", KEY_DOWN);
  this->controller_manager->add_action_key("Move Left", KEY_LEFT);
  this->controller_manager->add_action_key("Move Right", KEY_RIGHT);
  this->controller_manager->add_action_key("Shoot", KEY_Y);

  // Register scenes and start on Title Screen:
  this->scene_manager->add_scene("title", new TitleScene());
  this->scene_manager->add_scene("level_1", new LevelScene("levels/stage_1_1.lvl"));
  this->scene_manager->add_scene("level_2", new LevelScene("levels/stage_1_2.lvl"));
  this->scene_manager->switch_to("title");
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

  // SELECT button shuts down the game (like original START functionality):
  if (keys_pressed & KEY_SELECT) {
    this->is_running = false;
    return;
  }

  this->scene_manager->process_input(keys_pressed, keys_released, keys_held, touch);
}

void Game::update(void) {
  // Increment global tick counter (used by AnimationSystem and timed events)
  global_frame_counter++;

  this->scene_manager->update(this->delta_time);
}

void Game::render(void) {
  this->scene_manager->render();

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
    // Apply any queued scene switch safely at frame boundary:
    this->scene_manager->apply_pending_switch();

    this->process_input();
    this->update();
    this->render();
  }
}

void Game::destroy(void) {
  if (this->audio_streamer != nullptr) {
    this->audio_streamer->stop();
  }

  NF_UnloadRawSound(0);

  if (this->asset_manager != nullptr) {
    this->asset_manager->clear_assets();
  }

  delete instance;
  instance = nullptr;
}
