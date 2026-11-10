#include "../include/LevelScene.h"
#include "../include/Game.h"

LevelScene::LevelScene(void) {
  this->init_3D_sprites();
}

// Initialize positions and instantiate 3D sprites in NFlib:
void LevelScene::init_3D_sprites(void) {
  srand(0x1337);

  // Initialize pool of available 3D sprite slots for Menaces (slots 1 to 10):
  this->free_menace_slots.clear();

  for (u8 slot = 1; slot <= 10; slot++) {
    this->free_menace_slots.push_back(slot);
  }

  this->active_menaces.clear();
  this->menace_spawn_timer = 20;  // Spawn first enemy shortly after start.
  this->menaces_defeated_count = 0;

  // Initialize pool of available 3D sprite slots for bullets (slots 11 to 40):
  this->free_bullet_slots.clear();

  for (u8 slot = 11; slot <= 40; slot++) {
    this->free_bullet_slots.push_back(slot);
  }

  this->active_bullets.clear();

  NF_Sort3dSprites();  // Sort priorities.
}

void LevelScene::cleanup_entities(void) {
  Registry* registry = Game::get_instance()->get_registry();

  if (NF_3DSPRITE[0].inuse) {
    NF_Delete3dSprite(0);
  }

  if (this->player.is_alive()) {
    this->player.delete_entity();
  }

  for (auto& menace : this->active_menaces) {
    if (menace.has_component<SpriteComponent>()) {
      auto& sprite = menace.get_component<SpriteComponent>();

      if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
        NF_Delete3dSprite(sprite.id);
      }

      this->free_menace_slots.push_back(sprite.id);
    }

    menace.delete_entity();
  }

  this->active_menaces.clear();

  for (auto& bullet : this->active_bullets) {
    if (bullet.has_component<SpriteComponent>()) {
      auto& sprite = bullet.get_component<SpriteComponent>();

      if (sprite.id < NF_3DSPRITES && NF_3DSPRITE[sprite.id].inuse) {
        NF_Delete3dSprite(sprite.id);
      }

      this->free_bullet_slots.push_back(sprite.id);
    }

    bullet.delete_entity();
  }

  this->active_bullets.clear();

  if (registry != nullptr) {
    registry->update();
  }
}

void LevelScene::on_enter(void) {
  this->cleanup_entities();
  this->init_3D_sprites();

  this->spawn_player();

  this->player_lives = 1;
  this->is_paused = false;
  this->menaces_defeated_count = 0;
  this->menace_spawn_timer = 30;
  this->shoot_cooldown = 0;
  this->last_update_tick = 0;

  this->bg_buildings_scroll_x = 0;
  this->bg_buildings_last_chunk = 0;
  this->bg_front_beam_scroll_x = 0;

  u8 map_base = NF_TILEDBG_LAYERS[0][2].mapbase;
  u8 bg_slot = NF_TILEDBG_LAYERS[0][2].bgslot;

  if (NF_BUFFER_BGMAP[bg_slot] != nullptr) {
    void* vram_dest0 = reinterpret_cast<void*>(0x06000000 + (map_base * 2048));
    void* vram_dest1 = reinterpret_cast<void*>(0x06000000 + ((map_base + 1) * 2048));
    dmaCopyWords(3, NF_BUFFER_BGMAP[bg_slot], vram_dest0, 2048);
    dmaCopyWords(3, NF_BUFFER_BGMAP[bg_slot] + 2048, vram_dest1, 2048);
  }

  NF_ScrollBg(0, 3, 0, 0);
  NF_ScrollBg(0, 2, 0, 0);
  NF_ScrollBg(0, 1, 0, 0);

  NF_ShowBg(0, 1);
  NF_ShowBg(0, 2);
  NF_ShowBg(0, 3);

  consoleClear();

  // Start gameplay BGM streaming (Cancer_Dancer.raw, 22050 Hz 8-bit mono, looping):
  AudioStreamer* audio_streamer = Game::get_instance()->get_audio_streamer();
  if (audio_streamer != nullptr) {
    audio_streamer->play("bgm/Cancer_Dancer.raw", 22050, true);
  }
}

void LevelScene::on_exit(void) {
  NF_HideBg(0, 1);
  NF_HideBg(0, 2);
  NF_HideBg(0, 3);

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
      }

      else {
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

    // Emit click event using touch pixel coordinates (px, py):
    // game->get_event_manager()->emit_event<ClickEvent>(KEY_TOUCH, touch.px, touch.py);
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

  // Update background scrolling (1024 px circular buffer streaming):
  this->bg_buildings_scroll_x += 1;
  if (this->bg_buildings_scroll_x >= BUILDINGS_TOTAL_WIDTH) {
    this->bg_buildings_scroll_x = 0;
  }

  s32 current_chunk = this->bg_buildings_scroll_x / BUILDINGS_CHUNK_WIDTH;
  if (current_chunk != this->bg_buildings_last_chunk) {
    this->bg_buildings_last_chunk = current_chunk;

    // When current_chunk is even (0, 2), Block 0 is on-screen -> Block 1 is off-screen.
    // When current_chunk is odd (1, 3), Block 1 is on-screen -> Block 0 is off-screen.
    u8 target_block = (current_chunk % 2 == 0) ? 1 : 0;
    s32 upcoming_chunk = (current_chunk + 1) % BUILDINGS_TOTAL_CHUNKS;

    u8 map_base = NF_TILEDBG_LAYERS[0][2].mapbase;
    void* vram_dest = reinterpret_cast<void*>(0x06000000 + ((map_base + target_block) * 2048));

    u8 bg_slot = NF_TILEDBG_LAYERS[0][2].bgslot;
    const void* ram_src = reinterpret_cast<const void*>(NF_BUFFER_BGMAP[bg_slot] + (upcoming_chunk * 2048));

    dmaCopyWords(3, ram_src, vram_dest, 2048);
  }

  s32 hw_buildings_scroll_x = this->bg_buildings_scroll_x % 512;
  NF_ScrollBg(0, 2, hw_buildings_scroll_x, 0);

  // Update foreground beam scrolling (move Front_Beam from right to left):
  this->bg_front_beam_scroll_x += this->front_beam_speed;

  if (this->bg_front_beam_scroll_x >= 256) {
    this->bg_front_beam_scroll_x -= 256;
  }

  NF_ScrollBg(0, 1, this->bg_front_beam_scroll_x, 0);

  // Handle player movement and boundary limits (including shooting):
  this->update_player_input();

  // registry->get_system<ScriptSystem>().update(this->lua);  // TODO
  registry->get_system<AnimationSystem>().update();
  registry->get_system<CollisionSystem>().update(*event_manager);
  registry->get_system<MovementSystem>().update(delta_time);

  // If collision in this frame killed the player, return to title screen:
  if (!this->player.is_alive()) {
    game->get_scene_manager()->switch_to("title");
    return;
  }

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

void LevelScene::render(void) {
  Registry* registry = Game::get_instance()->get_registry();
  // Update 2D sprite positions in NFlib OAM buffers via ECS:
  registry->get_system<RenderSystem>().update();
  NF_Draw3dSprites();
}

void LevelScene::spawn_player(void) {
  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  AssetManager* asset_manager = game->get_asset_manager();

  if (this->player.is_alive()) {
    this->player.delete_entity();
    registry->update();
  }

  u16 player_gfx_id = asset_manager->get_gfx_id("Player_1");
  u16 player_pal_id = asset_manager->get_pal_id("Player_1");

  s16 player_start_x = 20;
  s16 player_start_y = (SCREEN_HEIGHT - PLAYER_HEIGHT) / 2;

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
  this->player.add_component<HealthComponent>(1, 1);

  registry->update();

  NF_Sort3dSprites();
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
  }

  else if (transform.position.x > static_cast<s32>(SCREEN_WIDTH - PLAYER_WIDTH)) {
    transform.position.x = static_cast<s32>(SCREEN_WIDTH - PLAYER_WIDTH);
  }

  if (transform.position.y < 0) {
    transform.position.y = 0;
  }

  else if (transform.position.y > static_cast<s32>(SCREEN_HEIGHT - PLAYER_HEIGHT)) {
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

void LevelScene::spawn_menace(void) {
  if (this->free_menace_slots.empty() || this->active_menaces.size() >= MAX_MENACES) {
    return;
  }

  Game* game = Game::get_instance();
  Registry* registry = game->get_registry();
  AssetManager* asset_manager = game->get_asset_manager();

  u8 slot = this->free_menace_slots.back();
  this->free_menace_slots.pop_back();

  u16 menace_gfx_id = asset_manager->get_gfx_id("Menace");
  u16 menace_pal_id = asset_manager->get_pal_id("Menace");

  // Spawn from the right edge of the screen at random height:
  s16 spawn_x = SCREEN_WIDTH;
  s16 max_y = SCREEN_HEIGHT - MENACE_HEIGHT;
  s16 spawn_y = rand() % (max_y + 1);

  // Random speed between 35 and 75 px/s:
  s32 speed = 35 + (rand() % 41);

  NF_Create3dSprite(slot, menace_gfx_id, menace_pal_id, spawn_x, spawn_y);

  Entity new_menace = registry->create_entity();
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

void LevelScene::update_menaces(void) {
  Registry* registry = Game::get_instance()->get_registry();

  // Spawn countdown:
  if (this->menace_spawn_timer > 0) {
    this->menace_spawn_timer--;
  }

  else {
    this->spawn_menace();
    this->menace_spawn_timer = 45 + (rand() % 45);  // Spawn next Menace in ~0.75-1.5 seconds
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

void LevelScene::update_ui(void) {
  if (this->last_update_tick != 0 && (global_frame_counter - this->last_update_tick < 5)) {
    return;
  }

  this->last_update_tick = global_frame_counter;

  printf("\x1b[1;2H==============================");
  printf("\x1b[2;2H     2D GAME ENGINE (NDS)       ");
  printf("\x1b[3;2H==============================");
  printf("\x1b[5;2HControls:");
  printf("\x1b[6;4H- D-Pad : Move Player");
  printf("\x1b[7;4H- Y : Shoot");
  printf("\x1b[8;4H- START : Pause / Resume");
  printf("\x1b[9;4H- SELECT: Shutdown Console");
  printf("\x1b[11;2H------------------------------");
  printf("\x1b[12;2HPlayer Lives   : %2d           ", this->player_lives);
  printf("\x1b[13;2HActive Menaces : %2d / %2d      ", static_cast<int>(this->active_menaces.size()), static_cast<int>(MAX_MENACES));
  printf("\x1b[14;2HDefeated Score : %4lu           ", this->menaces_defeated_count);
  printf("\x1b[15;2HActive Bullets : %2d            ", static_cast<int>(this->active_bullets.size()));

  if (this->is_paused) {
    printf("\x1b[17;7H*** GAME PAUSED ***    ");
    printf("\x1b[18;5HPress START to resume  ");
  }

  else {
    printf("\x1b[17;2H                              ");
    printf("\x1b[18;2H                              ");
  }
}

