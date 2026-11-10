#include "../include/LevelScene.h"
#include "../include/Game.h"

LevelScene::LevelScene(const std::string& level_file_path)
    : level_file_path(level_file_path) {
  this->init_3D_sprites();
}

void LevelScene::setup_default_level_data(void) {
  this->level_data = LevelData();
  this->level_data.name = "STAGE 1-1";
  this->level_data.next_scene = "title";
  this->level_data.target_score = 10;
  this->level_data.bgm = {"bgm/Cancer_Dancer.raw", 22050, true};

  this->level_data.backgrounds = {
    {3, "bg3", "bg/Stage_1-1", 256, 256, 3, 0, 0},
    {2, "buildings", "bg/buildings", 512, 256, 2, 1, 1024},
    {1, "Front_Beam", "bg/Front_Beam", 256, 256, 0, 3, 0}
  };

  this->level_data.sprite_assets = {
    {"Player_1", "sprite/Player_1", 64, 64, 0, 0, false},
    {"Menace", "sprite/Menace", 32, 32, 1, 1, false},
    {"Player_Bullet", "sprite/Player_Bullet", BULLET_WIDTH, BULLET_HEIGHT, 2, 2, false},
    {"blueball", "sprite/blueball", 64, 64, 3, 3, false}
  };
}

// Initialize pools of 3D sprite hardware slots in NFlib:
void LevelScene::init_3D_sprites(void) {
  srand(0x1337);

  // Pool of available 3D sprite slots for Enemies (slots 1 to 10):
  this->free_menace_slots.clear();
  for (u8 slot = 1; slot <= 10; slot++) {
    this->free_menace_slots.push_back(slot);
  }
  this->active_menaces.clear();
  this->menace_spawn_timer = 20;
  this->menaces_defeated_count = 0;

  // Pool of available 3D sprite slots for Bullets (slots 11 to 40):
  this->free_bullet_slots.clear();
  for (u8 slot = 11; slot <= 40; slot++) {
    this->free_bullet_slots.push_back(slot);
  }
  this->active_bullets.clear();

  // Pool of available 3D sprite slots for Furniture / Props (slots 41 to 55):
  this->free_prop_slots.clear();
  for (u8 slot = 41; slot <= 55; slot++) {
    this->free_prop_slots.push_back(slot);
  }
  this->active_props.clear();

  NF_Sort3dSprites();
}

void LevelScene::setup_level_assets_and_bgs(void) {
  AssetManager* asset_manager = Game::get_instance()->get_asset_manager();

  // Load 3D sprite assets required by this level:
  for (const auto& asset : this->level_data.sprite_assets) {
    asset_manager->load_3d_sprite(
      asset.sprite_id, asset.file_path,
      asset.width, asset.height,
      asset.gfx_slot, asset.pal_slot, asset.keepframes
    );
  }

  // Delete any previously created tiled backgrounds on Screen 0 layers 1..3:
  for (u8 layer = 1; layer <= 3; layer++) {
    if (NF_TILEDBG_LAYERS[0][layer].created) {
      NF_DeleteTiledBg(0, layer);
    }
  }

  this->bg_states.clear();

  // Load and create each background layer configured for this level:
  for (const auto& bg : this->level_data.backgrounds) {
    asset_manager->load_tiled_bg(bg.bg_id, bg.file_path, bg.width, bg.height);
    NF_CreateTiledBg(0, bg.layer, bg.bg_id.c_str());

    // Set hardware priority for this background layer:
    if (bg.layer == 1) {
      REG_BG1CNT = (REG_BG1CNT & ~3) | (bg.priority & 3);
    } else if (bg.layer == 2) {
      REG_BG2CNT = (REG_BG2CNT & ~3) | (bg.priority & 3);
    } else if (bg.layer == 3) {
      REG_BG3CNT = (REG_BG3CNT & ~3) | (bg.priority & 3);
    }

    // If this layer uses circular DMA streaming (> 512px wide), initialize its first two 256px chunks:
    if (bg.stream_total_width > 0) {
      u8 map_base = NF_TILEDBG_LAYERS[0][bg.layer].mapbase;
      u8 bg_slot = NF_TILEDBG_LAYERS[0][bg.layer].bgslot;

      if (NF_BUFFER_BGMAP[bg_slot] != nullptr) {
        void* vram_dest0 = reinterpret_cast<void*>(0x06000000 + (map_base * 2048));
        void* vram_dest1 = reinterpret_cast<void*>(0x06000000 + ((map_base + 1) * 2048));
        dmaCopyWords(3, NF_BUFFER_BGMAP[bg_slot], vram_dest0, 2048);
        dmaCopyWords(3, NF_BUFFER_BGMAP[bg_slot] + 2048, vram_dest1, 2048);
      }
    }

    NF_ScrollBg(0, bg.layer, 0, 0);
    NF_ShowBg(0, bg.layer);

    BgRuntimeState state;
    state.config = bg;
    state.scroll_x = 0;
    state.last_chunk = 0;
    this->bg_states.push_back(state);
  }

  // Layer 0 (3D Sprites) priority:
  REG_BG0CNT = (REG_BG0CNT & ~3) | (this->level_data.sprite_layer_priority & 3);
}

void LevelScene::cleanup_entities(void) {
  Registry* registry = Game::get_instance()->get_registry();

  if (NF_3DSPRITE[0].inuse) {
    NF_Delete3dSprite(0);
  }

  if (this->player.is_alive()) {
    this->player.remove_component<SpriteComponent>();
    this->player.delete_entity();
  }
  this->player = Entity();

  for (auto& menace : this->active_menaces) {
    if (menace.has_component<SpriteComponent>()) {
      auto& sprite = menace.get_component<SpriteComponent>();
      u8 slot = sprite.id;

      if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
        NF_Delete3dSprite(slot);
      }

      menace.remove_component<SpriteComponent>();
      this->free_menace_slots.push_back(slot);
    }

    menace.delete_entity();
  }
  this->active_menaces.clear();

  for (auto& bullet : this->active_bullets) {
    if (bullet.has_component<SpriteComponent>()) {
      auto& sprite = bullet.get_component<SpriteComponent>();
      u8 slot = sprite.id;

      if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
        NF_Delete3dSprite(slot);
      }

      bullet.remove_component<SpriteComponent>();
      this->free_bullet_slots.push_back(slot);
    }

    bullet.delete_entity();
  }
  this->active_bullets.clear();

  for (auto& prop : this->active_props) {
    if (prop.has_component<SpriteComponent>()) {
      auto& sprite = prop.get_component<SpriteComponent>();
      u8 slot = sprite.id;

      if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
        NF_Delete3dSprite(slot);
      }

      prop.remove_component<SpriteComponent>();
      this->free_prop_slots.push_back(slot);
    }

    prop.delete_entity();
  }
  this->active_props.clear();

  if (registry != nullptr) {
    registry->update();
  }
}

void LevelScene::on_enter(void) {
  this->cleanup_entities();
  this->init_3D_sprites();

  // Load level definition from NitroFS (or fall back to default Stage 1-1 data):
  if (!LevelLoader::load_from_file(this->level_file_path, this->level_data)) {
    this->setup_default_level_data();
  }

  this->setup_level_assets_and_bgs();

  this->player_lives = this->level_data.player.lives;
  this->player_speed = this->level_data.player.speed;
  this->is_paused = false;

  this->level_scroll_distance = 0;
  this->next_prop_index = 0;
  this->next_enemy_index = 0;

  this->menaces_defeated_count = 0;
  this->menace_spawn_timer = 30;
  this->shoot_cooldown = 0;
  this->last_update_tick = 0;

  this->spawn_player();

  consoleClear();

  // Start level BGM streaming if configured:
  AudioStreamer* audio_streamer = Game::get_instance()->get_audio_streamer();
  if (audio_streamer != nullptr && !this->level_data.bgm.file_path.empty()) {
    audio_streamer->play(
      this->level_data.bgm.file_path,
      this->level_data.bgm.sample_rate,
      this->level_data.bgm.loop
    );
  }
}

void LevelScene::on_exit(void) {
  for (u8 layer = 1; layer <= 3; layer++) {
    NF_HideBg(0, layer);
    if (NF_TILEDBG_LAYERS[0][layer].created) {
      NF_DeleteTiledBg(0, layer);
    }
  }

  this->bg_states.clear();
  this->cleanup_entities();

  // Stop gameplay BGM when leaving the level scene:
  AudioStreamer* audio_streamer = Game::get_instance()->get_audio_streamer();
  if (audio_streamer != nullptr) {
    audio_streamer->stop();
  }

  consoleClear();
}

void LevelScene::process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) {
  Game* game = Game::get_instance();
  AudioStreamer* audio_streamer = game->get_audio_streamer();

  // During Gameplay: START button pauses / un-pauses the game:
  if (keys_pressed & KEY_START) {
    this->is_paused = !this->is_paused;

    if (audio_streamer != nullptr) {
      if (this->is_paused) {
        audio_streamer->pause();
      } else {
        audio_streamer->resume();
      }
    }

    return;
  }

  // If paused, don't process gameplay movement/shooting:
  if (this->is_paused) {
    return;
  }

  // Update ControllerManager's key states:
  game->controller_manager->update();

  // Touch Screen / Mouse Events:
  if (keys_held & KEY_TOUCH) {
    game->controller_manager->set_mouse_position(touch.px, touch.py);
  }

  if (keys_pressed & KEY_TOUCH) {
    game->controller_manager->set_mouse_position(touch.px, touch.py);
    game->controller_manager->set_mouse_button_down(KEY_TOUCH);
  }

  if (keys_released & KEY_TOUCH) {
    game->controller_manager->set_mouse_position(touch.px, touch.py);
    game->controller_manager->set_mouse_button_up(KEY_TOUCH);
  }
}

void LevelScene::update(float delta_time) {
  if (this->is_paused) {
    this->update_ui();
    return;
  }

  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  EventManager* event_manager = game->get_event_manager();

  // Update the engine systems:
  event_manager->reset();
  registry->get_system<DamageSystem>().subscribe_to_collision_event(*event_manager);

  registry->update();

  // If player died from collision / damage, return to title screen:
  if (!this->player.is_alive()) {
    game->get_scene_manager()->switch_to("title");
    return;
  }

  // Check if player reached the level's target score to advance to next_scene:
  if (this->level_data.target_score > 0 &&
      this->menaces_defeated_count >= this->level_data.target_score) {
    game->get_scene_manager()->switch_to(this->level_data.next_scene);
    return;
  }

  // Advance level scroll distance and update background layers:
  this->level_scroll_distance++;
  this->update_backgrounds();

  // Spawn any scripted furniture/props or enemies triggered at current scroll distance:
  this->spawn_scripted_entities();

  // Handle player movement and boundary limits (including shooting):
  this->update_player_input();

  // Add newly spawned entities to systems before running system updates:
  registry->update();

  registry->get_system<AnimationSystem>().update();
  registry->get_system<CollisionSystem>().update(*event_manager);
  registry->get_system<MovementSystem>().update(delta_time);

  // If collision in this frame killed the player, return to title screen:
  if (!this->player.is_alive()) {
    game->get_scene_manager()->switch_to("title");
    return;
  }

  // Update furniture/props, enemy spawns & homing, and bullets:
  this->update_props();
  this->update_menaces();
  this->update_bullets();

  // Flush entity removals/additions before rendering:
  registry->update();

  // Update bottom screen stats:
  this->update_ui();

  // Push updated sprite transformations to OAM VRAM:
  NF_SpriteOamSet(0);  // Top screen OAM update.
  NF_SpriteOamSet(1);  // Bottom screen OAM update.
}

void LevelScene::update_backgrounds(void) {
  static constexpr s32 CHUNK_WIDTH = 256;

  for (auto& bg : this->bg_states) {
    if (bg.config.scroll_speed_x == 0) {
      continue;
    }

    bg.scroll_x += bg.config.scroll_speed_x;

    if (bg.config.stream_total_width > 0) {
      // Circular buffer DMA streaming for maps wider than 512px:
      if (bg.scroll_x >= bg.config.stream_total_width) {
        bg.scroll_x -= bg.config.stream_total_width;
      }

      s32 total_chunks = bg.config.stream_total_width / CHUNK_WIDTH;
      s32 current_chunk = bg.scroll_x / CHUNK_WIDTH;

      if (current_chunk != bg.last_chunk && total_chunks > 0) {
        bg.last_chunk = current_chunk;

        u8 target_block = (current_chunk % 2 == 0) ? 1 : 0;
        s32 upcoming_chunk = (current_chunk + 1) % total_chunks;

        u8 map_base = NF_TILEDBG_LAYERS[0][bg.config.layer].mapbase;
        void* vram_dest = reinterpret_cast<void*>(0x06000000 + ((map_base + target_block) * 2048));

        u8 bg_slot = NF_TILEDBG_LAYERS[0][bg.config.layer].bgslot;
        const void* ram_src = reinterpret_cast<const void*>(NF_BUFFER_BGMAP[bg_slot] + (upcoming_chunk * 2048));

        dmaCopyWords(3, ram_src, vram_dest, 2048);
      }

      NF_ScrollBg(0, bg.config.layer, bg.scroll_x % 512, 0);
    } else {
      // Standard hardware wrapping scroll:
      s32 wrap_w = (bg.config.width > 0) ? bg.config.width : 256;
      if (bg.scroll_x >= wrap_w) {
        bg.scroll_x -= wrap_w;
      }
      NF_ScrollBg(0, bg.config.layer, bg.scroll_x, 0);
    }
  }
}

void LevelScene::render(void) {
  Registry* registry = Game::get_instance()->get_registry();
  // Update 2D/3D sprite positions in NFlib buffers via ECS:
  registry->get_system<RenderSystem>().update();
  NF_Draw3dSprites();
}

void LevelScene::spawn_player(void) {
  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  AssetManager* asset_manager = game->get_asset_manager();

  if (this->player.is_alive()) {
    this->player.remove_component<SpriteComponent>();
    this->player.delete_entity();
    registry->update();
  }

  const std::string& sprite_id = this->level_data.player.sprite_id;
  u16 player_gfx_id = asset_manager->get_gfx_id(sprite_id);
  u16 player_pal_id = asset_manager->get_pal_id(sprite_id);

  s16 player_start_x = this->level_data.player.start_x;
  s16 player_start_y = this->level_data.player.start_y;

  if (NF_3DSPRITE[0].inuse) {
    NF_Delete3dSprite(0);
  }

  NF_Create3dSprite(0, player_gfx_id, player_pal_id, player_start_x, player_start_y);

  this->player = registry->create_entity();
  this->player.add_component<AnimationComponent>(2, 8, true);
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

  this->player.add_component<CircleColliderComponent>(16, PLAYER_WIDTH, PLAYER_HEIGHT);
  this->player.add_component<HealthComponent>(this->player_lives, this->player_lives);

  registry->update();

  NF_Sort3dSprites();
}

void LevelScene::spawn_scripted_entities(void) {
  // Spawn props whose trigger_distance has been reached:
  while (this->next_prop_index < this->level_data.props.size()) {
    const auto& prop_cfg = this->level_data.props[this->next_prop_index];
    if (prop_cfg.trigger_distance > this->level_scroll_distance) {
      break;
    }
    this->spawn_prop(prop_cfg);
    this->next_prop_index++;
  }

  // Spawn scripted enemies whose trigger_distance has been reached:
  while (this->next_enemy_index < this->level_data.enemies.size()) {
    const auto& enemy_cfg = this->level_data.enemies[this->next_enemy_index];
    if (enemy_cfg.trigger_distance > this->level_scroll_distance) {
      break;
    }
    this->spawn_enemy_from_config(enemy_cfg);
    this->next_enemy_index++;
  }
}

void LevelScene::spawn_prop(const PropSpawnConfig& cfg) {
  if (this->free_prop_slots.empty()) {
    return;
  }

  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  AssetManager* asset_manager = game->get_asset_manager();

  u16 gfx_id = asset_manager->get_gfx_id(cfg.sprite_id);
  u16 pal_id = asset_manager->get_pal_id(cfg.sprite_id);
  if (gfx_id == 255 || pal_id == 255) {
    return;
  }

  u8 slot = this->free_prop_slots.back();
  this->free_prop_slots.pop_back();

  NF_Create3dSprite(slot, gfx_id, pal_id, cfg.x, cfg.y);

  Entity prop = registry->create_entity();
  prop.add_component<TransformComponent>(Vec2f(cfg.x, cfg.y));
  prop.add_component<RigidBodyComponent>(Vec2f(cfg.velocity_x, cfg.velocity_y));
  prop.add_component<SpriteComponent>(
    0, slot, gfx_id, pal_id, cfg.width, cfg.height, true, false
  );
  prop.add_component<CircleColliderComponent>(cfg.collider_radius, cfg.width, cfg.height);

  bool destructible = (cfg.hp > 0);
  prop.add_component<PropComponent>(destructible, true);

  if (destructible) {
    prop.add_component<HealthComponent>(cfg.hp, cfg.hp);
  }

  this->active_props.push_back(prop);
  NF_Sort3dSprites();
}

void LevelScene::update_props(void) {
  Registry* registry = Game::get_instance()->get_registry();

  for (auto it = this->active_props.begin(); it != this->active_props.end(); ) {
    Entity prop = *it;

    if (registry->is_entity_to_remove(prop) || !prop.has_component<TransformComponent>()) {
      if (prop.has_component<SpriteComponent>()) {
        auto& sprite = prop.get_component<SpriteComponent>();
        u8 slot = sprite.id;
        if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
          NF_Delete3dSprite(slot);
        }
        prop.remove_component<SpriteComponent>();
        this->free_prop_slots.push_back(slot);
      }
      it = this->active_props.erase(it);
      continue;
    }

    auto& transform = prop.get_component<TransformComponent>();
    if (transform.position.x < -64 || transform.position.x > static_cast<s32>(SCREEN_WIDTH + 64) ||
        transform.position.y < -64 || transform.position.y > static_cast<s32>(SCREEN_HEIGHT + 64)) {
      if (prop.has_component<SpriteComponent>()) {
        auto& sprite = prop.get_component<SpriteComponent>();
        u8 slot = sprite.id;
        if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
          NF_Delete3dSprite(slot);
        }
        prop.remove_component<SpriteComponent>();
        this->free_prop_slots.push_back(slot);
      }
      prop.delete_entity();
      it = this->active_props.erase(it);
      continue;
    }

    ++it;
  }
}

void LevelScene::update_player_input(void) {
  if (!this->player.has_component<TransformComponent>()) {
    return;
  }

  ControllerManager* controller_manager = Game::get_instance()->controller_manager;
  auto& transform = this->player.get_component<TransformComponent>();

  // Apply movement based on D-pad input via ControllerManager and hardware keys:
  if (controller_manager->is_action_activated("Move Left") || (keys & KEY_LEFT)) {
    transform.position.x -= this->player_speed;
  }

  if (controller_manager->is_action_activated("Move Right") || (keys & KEY_RIGHT)) {
    transform.position.x += this->player_speed;
  }

  if (controller_manager->is_action_activated("Move Up") || (keys & KEY_UP)) {
    transform.position.y -= this->player_speed;
  }

  if (controller_manager->is_action_activated("Move Down") || (keys & KEY_DOWN)) {
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

  if (controller_manager->is_action_activated("Shoot") || (keys & (KEY_Y))) {
    if (this->shoot_cooldown == 0) {
      this->shoot_bullet();
      this->shoot_cooldown = 12;  // Rate limit between shots (~5 shots/sec).
    }
  }
}

void LevelScene::shoot_bullet(void) {
  if (this->free_bullet_slots.empty()) {
    return;
  }

  if (!this->player.has_component<TransformComponent>()) {
    return;
  }

  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  AssetManager* asset_manager = game->get_asset_manager();

  u8 slot = this->free_bullet_slots.back();
  this->free_bullet_slots.pop_back();

  const auto& player_transform = this->player.get_component<TransformComponent>();

  u16 bullet_gfx_id = asset_manager->get_gfx_id("Player_Bullet");
  u16 bullet_pal_id = asset_manager->get_pal_id("Player_Bullet");

  // Spawn bullet in front of the player ship:
  s16 bullet_start_x = player_transform.position.x + 34;
  s16 bullet_start_y = player_transform.position.y + (PLAYER_HEIGHT - 49);

  NF_Create3dSprite(slot, bullet_gfx_id, bullet_pal_id, bullet_start_x, bullet_start_y);

  Entity bullet = registry->create_entity();
  bullet.add_component<TransformComponent>(Vec2f(bullet_start_x, bullet_start_y));
  bullet.add_component<RigidBodyComponent>(Vec2f(240, 0));  // 240 px/sec (4 pixels per frame)
  bullet.add_component<SpriteComponent>(
    0,               // Screen (0 = Top Screen).
    slot,            // 3D Sprite hardware slot ID.
    bullet_gfx_id,   // Loaded GFX RAM/VRAM slot.
    bullet_pal_id,   // Loaded Palette RAM/VRAM slot.
    BULLET_WIDTH,    // Width (8).
    BULLET_HEIGHT,   // Height (8).
    true,            // is_3D.
    false            // is_rotscale.
  );

  bullet.add_component<AnimationComponent>(
    2,               // 2 frames.
    8,               // Switch frame every 8 frames.
    true             // Loop animation.
  );

  bullet.add_component<CircleColliderComponent>(4, BULLET_WIDTH, BULLET_HEIGHT);
  bullet.add_component<ProjectileComponent>(20, true);

  this->active_bullets.push_back(bullet);
}

void LevelScene::update_bullets(void) {
  Registry* registry = Game::get_instance()->get_registry();

  for (auto it = this->active_bullets.begin(); it != this->active_bullets.end(); ) {
    Entity bullet = *it;

    // Check if bullet was destroyed by DamageSystem upon collision:
    if (registry->is_entity_to_remove(bullet) || !bullet.has_component<TransformComponent>()) {
      if (bullet.has_component<SpriteComponent>()) {
        auto& sprite = bullet.get_component<SpriteComponent>();
        u8 slot = sprite.id;

        if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
          NF_Delete3dSprite(slot);
        }

        bullet.remove_component<SpriteComponent>();
        this->free_bullet_slots.push_back(slot);
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
        u8 slot = sprite.id;

        if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
          NF_Delete3dSprite(slot);
        }

        bullet.remove_component<SpriteComponent>();
        this->free_bullet_slots.push_back(slot);
      }

      bullet.delete_entity();
      it = this->active_bullets.erase(it);
      continue;
    }

    ++it;
  }
}

void LevelScene::spawn_enemy_from_config(const EnemySpawnConfig& cfg) {
  if (this->free_menace_slots.empty() ||
      this->active_menaces.size() >= this->level_data.spawner.max_active) {
    return;
  }

  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  AssetManager* asset_manager = game->get_asset_manager();

  u16 gfx_id = asset_manager->get_gfx_id(cfg.sprite_id);
  u16 pal_id = asset_manager->get_pal_id(cfg.sprite_id);
  if (gfx_id == 255 || pal_id == 255) {
    return;
  }

  u8 slot = this->free_menace_slots.back();
  this->free_menace_slots.pop_back();

  NF_Create3dSprite(slot, gfx_id, pal_id, cfg.x, cfg.y);

  Entity new_enemy = registry->create_entity();
  new_enemy.add_component<TransformComponent>(Vec2f(cfg.x, cfg.y));
  new_enemy.add_component<RigidBodyComponent>(Vec2f(-cfg.speed, 0));
  new_enemy.add_component<SpriteComponent>(
    0, slot, gfx_id, pal_id, cfg.width, cfg.height, true, false
  );

  if (cfg.num_frames > 1) {
    new_enemy.add_component<AnimationComponent>(cfg.num_frames, cfg.anim_speed, true);
  }

  new_enemy.add_component<CircleColliderComponent>(cfg.collider_radius, cfg.width, cfg.height);
  new_enemy.add_component<HealthComponent>(cfg.hp, cfg.hp);
  new_enemy.add_component<EnemyComponent>(cfg.speed, cfg.homing);

  this->active_menaces.push_back(new_enemy);
  NF_Sort3dSprites();
}

void LevelScene::spawn_random_menace(void) {
  const auto& spawner = this->level_data.spawner;
  if (!spawner.enabled) {
    return;
  }

  s16 max_y = SCREEN_HEIGHT - MENACE_HEIGHT;
  s32 speed_range = (spawner.max_speed > spawner.min_speed)
                        ? (spawner.max_speed - spawner.min_speed + 1)
                        : 1;

  EnemySpawnConfig cfg;
  cfg.sprite_id = spawner.sprite_id;
  cfg.x = SCREEN_WIDTH;
  cfg.y = rand() % (max_y + 1);
  cfg.width = MENACE_WIDTH;
  cfg.height = MENACE_HEIGHT;
  cfg.num_frames = 4;
  cfg.anim_speed = 30;
  cfg.collider_radius = 16;
  cfg.speed = spawner.min_speed + (rand() % speed_range);
  cfg.hp = spawner.hp;
  cfg.homing = true;

  this->spawn_enemy_from_config(cfg);
}

void LevelScene::update_menaces(void) {
  Registry* registry = Game::get_instance()->get_registry();
  const auto& spawner = this->level_data.spawner;

  // Continuous random spawner countdown (if enabled in level file):
  if (spawner.enabled) {
    if (this->menace_spawn_timer > 0) {
      this->menace_spawn_timer--;
    } else {
      this->spawn_random_menace();
      u16 range = (spawner.max_interval > spawner.min_interval)
                      ? (spawner.max_interval - spawner.min_interval)
                      : 1;
      this->menace_spawn_timer = spawner.min_interval + (rand() % range);
    }
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
    if (registry->is_entity_to_remove(menace_entity) || !menace_entity.has_component<TransformComponent>()) {
      if (menace_entity.has_component<SpriteComponent>()) {
        auto& sprite = menace_entity.get_component<SpriteComponent>();
        u8 slot = sprite.id;

        if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
          NF_Delete3dSprite(slot);
        }

        menace_entity.remove_component<SpriteComponent>();
        this->free_menace_slots.push_back(slot);
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
        u8 slot = sprite.id;

        if (slot < NF_3DSPRITES && NF_3DSPRITE[slot].inuse) {
          NF_Delete3dSprite(slot);
        }

        menace_entity.remove_component<SpriteComponent>();
        this->free_menace_slots.push_back(slot);
      }

      menace_entity.delete_entity();
      it = this->active_menaces.erase(it);
      continue;
    }

    // Move to the player position at the enemy's individual speed if homing is enabled:
    if (has_player && menace_entity.has_component<RigidBodyComponent>() && menace_entity.has_component<EnemyComponent>()) {
      const auto& enemy = menace_entity.get_component<EnemyComponent>();
      if (enemy.homing) {
        auto& rb = menace_entity.get_component<RigidBodyComponent>();

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
    }

    ++it;
  }
}

void LevelScene::update_ui(void) {
  if (this->last_update_tick != 0 && (global_frame_counter - this->last_update_tick < 5)) {
    return;
  }

  this->last_update_tick = global_frame_counter;

  printf("\x1b[1;2H==============================");
  printf("\x1b[2;2H %-28.28s ", this->level_data.name.c_str());
  printf("\x1b[3;2H==============================");
  printf("\x1b[5;2HControls:");
  printf("\x1b[6;4H- D-Pad : Move Player");
  printf("\x1b[7;4H- Y     : Shoot");
  printf("\x1b[8;4H- START : Pause / Resume");
  printf("\x1b[9;4H- SELECT: Shutdown Console");
  printf("\x1b[11;2H------------------------------");
  printf("\x1b[12;2HPlayer Lives   : %2d           ", this->player_lives);
  printf("\x1b[13;2HActive Enemies : %2d / %2d      ",
         static_cast<int>(this->active_menaces.size()),
         static_cast<int>(this->level_data.spawner.max_active));
  printf("\x1b[14;2HDefeated Goal  : %2lu / %2lu      ",
         this->menaces_defeated_count,
         this->level_data.target_score);
  printf("\x1b[15;2HActive Props   : %2d            ", static_cast<int>(this->active_props.size()));

  if (this->is_paused) {
    printf("\x1b[17;7H*** GAME PAUSED ***    ");
    printf("\x1b[18;5HPress START to resume  ");
  } else {
    printf("\x1b[17;2H                              ");
    printf("\x1b[18;2H                              ");
  }
}
