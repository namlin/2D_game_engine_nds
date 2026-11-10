#include "../include/LevelLoader.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

static FILE* open_level_file(const std::string& path, const char* mode) {
  FILE* f = fopen(path.c_str(), mode);
  if (f != nullptr) {
    return f;
  }

  std::string nitro_path = "nitro:/" + path;
  f = fopen(nitro_path.c_str(), mode);
  if (f != nullptr) {
    return f;
  }

  std::string slash_path = "/" + path;
  return fopen(slash_path.c_str(), mode);
}

bool LevelLoader::load_from_file(const std::string& file_path, LevelData& out_data) {
  FILE* file = open_level_file(file_path, "r");
  if (file == nullptr) {
    return false;
  }

  out_data = LevelData();
  out_data.spawner.enabled = false;

  char line[256];
  while (fgets(line, sizeof(line), file) != nullptr) {
    // Strip trailing newline/carriage return:
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
      line[--len] = '\0';
    }

    // Skip leading whitespace:
    char* ptr = line;
    while (*ptr == ' ' || *ptr == '\t') {
      ptr++;
    }

    // Skip empty lines and comments:
    if (*ptr == '\0' || *ptr == '#') {
      continue;
    }

    char cmd[32] = {};
    if (sscanf(ptr, "%31s", cmd) != 1) {
      continue;
    }

    if (strcmp(cmd, "NAME") == 0) {
      const char* name_start = ptr + 4;
      while (*name_start == ' ' || *name_start == '\t') {
        name_start++;
      }
      out_data.name = name_start;
    }

    else if (strcmp(cmd, "NEXT_SCENE") == 0) {
      char next_buf[64] = {};
      if (sscanf(ptr, "%*s %63s", next_buf) == 1) {
        out_data.next_scene = next_buf;
      }
    }

    else if (strcmp(cmd, "TARGET_SCORE") == 0) {
      unsigned int target = 0;
      if (sscanf(ptr, "%*s %u", &target) == 1) {
        out_data.target_score = static_cast<u32>(target);
      }
    }

    else if (strcmp(cmd, "BGM") == 0) {
      char path_buf[128] = {};
      unsigned int rate = 22050;
      int loop_int = 1;
      if (sscanf(ptr, "%*s %127s %u %d", path_buf, &rate, &loop_int) >= 1) {
        out_data.bgm.file_path = path_buf;
        out_data.bgm.sample_rate = rate;
        out_data.bgm.loop = (loop_int != 0);
      }
    }

    else if (strcmp(cmd, "BG") == 0) {
      // BG <layer> <bg_id> <file_path> <width> <height> <priority> <scroll_speed_x> <stream_total_width>
      unsigned int layer = 3, width = 256, height = 256, prio = 3;
      int scroll_speed = 0, stream_w = 0;
      char id_buf[64] = {};
      char path_buf[128] = {};
      int matched = sscanf(ptr, "%*s %u %63s %127s %u %u %u %d %d",
                           &layer, id_buf, path_buf, &width, &height, &prio, &scroll_speed, &stream_w);
      if (matched >= 7) {
        BgLayerConfig bg;
        bg.layer = static_cast<u8>(layer);
        bg.bg_id = id_buf;
        bg.file_path = path_buf;
        bg.width = static_cast<u16>(width);
        bg.height = static_cast<u16>(height);
        bg.priority = static_cast<u8>(prio);
        bg.scroll_speed_x = scroll_speed;
        bg.stream_total_width = (matched >= 8) ? stream_w : 0;
        out_data.backgrounds.push_back(bg);
      }
    }

    else if (strcmp(cmd, "ASSET") == 0) {
      // ASSET <sprite_id> <file_path> <width> <height> <gfx_slot> <pal_slot> [keepframes]
      char id_buf[64] = {};
      char path_buf[128] = {};
      unsigned int w = 32, h = 32, gfx = 0, pal = 0;
      int keep = 0;
      int matched = sscanf(ptr, "%*s %63s %127s %u %u %u %u %d",
                           id_buf, path_buf, &w, &h, &gfx, &pal, &keep);
      if (matched >= 6) {
        SpriteAssetConfig asset;
        asset.sprite_id = id_buf;
        asset.file_path = path_buf;
        asset.width = static_cast<u16>(w);
        asset.height = static_cast<u16>(h);
        asset.gfx_slot = static_cast<u16>(gfx);
        asset.pal_slot = static_cast<u16>(pal);
        asset.keepframes = (keep != 0);
        out_data.sprite_assets.push_back(asset);
      }
    }

    else if (strcmp(cmd, "PLAYER") == 0) {
      // PLAYER <sprite_id> <start_x> <start_y> <speed> <lives>
      char id_buf[64] = {};
      int x = 20, y = 64, speed = 2, lives = 1;
      if (sscanf(ptr, "%*s %63s %d %d %d %d", id_buf, &x, &y, &speed, &lives) >= 3) {
        out_data.player.sprite_id = id_buf;
        out_data.player.start_x = static_cast<s16>(x);
        out_data.player.start_y = static_cast<s16>(y);
        out_data.player.speed = static_cast<s16>(speed);
        out_data.player.lives = lives;
      }
    }

    else if (strcmp(cmd, "PROP") == 0) {
      // PROP <trigger_dist> <sprite_id> <x> <y> <w> <h> <radius> <vx> <vy> <hp>
      int trigger = 0, x = 0, y = 0, vx = -60, vy = 0, hp = 0;
      unsigned int w = 32, h = 32, radius = 16;
      char id_buf[64] = {};
      if (sscanf(ptr, "%*s %d %63s %d %d %u %u %u %d %d %d",
                 &trigger, id_buf, &x, &y, &w, &h, &radius, &vx, &vy, &hp) >= 7) {
        PropSpawnConfig prop;
        prop.trigger_distance = trigger;
        prop.sprite_id = id_buf;
        prop.x = static_cast<s16>(x);
        prop.y = static_cast<s16>(y);
        prop.width = static_cast<u16>(w);
        prop.height = static_cast<u16>(h);
        prop.collider_radius = static_cast<u16>(radius);
        prop.velocity_x = vx;
        prop.velocity_y = vy;
        prop.hp = hp;
        out_data.props.push_back(prop);
      }
    }

    else if (strcmp(cmd, "ENEMY") == 0) {
      // ENEMY <trigger_dist> <sprite_id> <x> <y> <w> <h> <frames> <anim_speed> <radius> <speed> <hp> <homing>
      int trigger = 0, x = 256, y = 80, speed = 50, hp = 40, homing = 1;
      unsigned int w = 32, h = 32, frames = 4, anim_spd = 30, radius = 16;
      char id_buf[64] = {};
      if (sscanf(ptr, "%*s %d %63s %d %d %u %u %u %u %u %d %d %d",
                 &trigger, id_buf, &x, &y, &w, &h, &frames, &anim_spd, &radius, &speed, &hp, &homing) >= 11) {
        EnemySpawnConfig enemy;
        enemy.trigger_distance = trigger;
        enemy.sprite_id = id_buf;
        enemy.x = static_cast<s16>(x);
        enemy.y = static_cast<s16>(y);
        enemy.width = static_cast<u16>(w);
        enemy.height = static_cast<u16>(h);
        enemy.num_frames = static_cast<u8>(frames);
        enemy.anim_speed = static_cast<u8>(anim_spd);
        enemy.collider_radius = static_cast<u16>(radius);
        enemy.speed = speed;
        enemy.hp = hp;
        enemy.homing = (homing != 0);
        out_data.enemies.push_back(enemy);
      }
    }

    else if (strcmp(cmd, "SPAWNER") == 0) {
      // SPAWNER <sprite_id> <min_interval> <max_interval> <max_active> <min_speed> <max_speed> <hp>
      char id_buf[64] = {};
      unsigned int min_int = 45, max_int = 90, max_act = 6;
      int min_spd = 35, max_spd = 75, hp = 40;
      if (sscanf(ptr, "%*s %63s %u %u %u %d %d %d",
                 id_buf, &min_int, &max_int, &max_act, &min_spd, &max_spd, &hp) >= 7) {
        out_data.spawner.enabled = true;
        out_data.spawner.sprite_id = id_buf;
        out_data.spawner.min_interval = static_cast<u16>(min_int);
        out_data.spawner.max_interval = static_cast<u16>(max_int);
        out_data.spawner.max_active = static_cast<u8>(max_act);
        out_data.spawner.min_speed = min_spd;
        out_data.spawner.max_speed = max_spd;
        out_data.spawner.hp = hp;
      }
    }
  }

  fclose(file);

  // Ensure props and enemies are sorted by trigger_distance ascending:
  std::sort(out_data.props.begin(), out_data.props.end(),
            [](const PropSpawnConfig& a, const PropSpawnConfig& b) {
              return a.trigger_distance < b.trigger_distance;
            });

  std::sort(out_data.enemies.begin(), out_data.enemies.end(),
            [](const EnemySpawnConfig& a, const EnemySpawnConfig& b) {
              return a.trigger_distance < b.trigger_distance;
            });

  return true;
}

bool LevelLoader::save_to_file(const std::string& file_path, const LevelData& data) {
  FILE* file = fopen(file_path.c_str(), "w");
  if (file == nullptr) {
    return false;
  }

  fprintf(file, "# Level Definition File\n");
  fprintf(file, "NAME %s\n", data.name.c_str());
  fprintf(file, "NEXT_SCENE %s\n", data.next_scene.c_str());
  fprintf(file, "TARGET_SCORE %lu\n\n", static_cast<unsigned long>(data.target_score));

  if (!data.bgm.file_path.empty()) {
    fprintf(file, "BGM %s %lu %d\n\n",
            data.bgm.file_path.c_str(),
            static_cast<unsigned long>(data.bgm.sample_rate),
            data.bgm.loop ? 1 : 0);
  }

  fprintf(file, "# Backgrounds: BG <layer> <id> <path> <w> <h> <prio> <scroll_speed> <stream_w>\n");
  for (const auto& bg : data.backgrounds) {
    fprintf(file, "BG %u %s %s %u %u %u %ld %ld\n",
            bg.layer, bg.bg_id.c_str(), bg.file_path.c_str(),
            bg.width, bg.height, bg.priority,
            static_cast<long>(bg.scroll_speed_x),
            static_cast<long>(bg.stream_total_width));
  }
  fprintf(file, "\n");

  fprintf(file, "# 3D Sprite Assets: ASSET <id> <path> <w> <h> <gfx_slot> <pal_slot> <keepframes>\n");
  for (const auto& asset : data.sprite_assets) {
    fprintf(file, "ASSET %s %s %u %u %u %u %d\n",
            asset.sprite_id.c_str(), asset.file_path.c_str(),
            asset.width, asset.height, asset.gfx_slot, asset.pal_slot,
            asset.keepframes ? 1 : 0);
  }
  fprintf(file, "\n");

  fprintf(file, "# Player: PLAYER <sprite_id> <start_x> <start_y> <speed> <lives>\n");
  fprintf(file, "PLAYER %s %d %d %d %d\n\n",
          data.player.sprite_id.c_str(),
          data.player.start_x, data.player.start_y,
          data.player.speed, data.player.lives);

  if (data.spawner.enabled) {
    fprintf(file, "# Continuous Spawner: SPAWNER <id> <min_int> <max_int> <max_act> <min_spd> <max_spd> <hp>\n");
    fprintf(file, "SPAWNER %s %u %u %u %ld %ld %d\n\n",
            data.spawner.sprite_id.c_str(),
            data.spawner.min_interval, data.spawner.max_interval, data.spawner.max_active,
            static_cast<long>(data.spawner.min_speed), static_cast<long>(data.spawner.max_speed),
            data.spawner.hp);
  }

  fprintf(file, "# Furniture / Props: PROP <trigger_dist> <id> <x> <y> <w> <h> <radius> <vx> <vy> <hp>\n");
  for (const auto& prop : data.props) {
    fprintf(file, "PROP %ld %s %d %d %u %u %u %ld %ld %d\n",
            static_cast<long>(prop.trigger_distance), prop.sprite_id.c_str(),
            prop.x, prop.y, prop.width, prop.height, prop.collider_radius,
            static_cast<long>(prop.velocity_x), static_cast<long>(prop.velocity_y), prop.hp);
  }
  fprintf(file, "\n");

  fprintf(file, "# Scripted Enemies: ENEMY <trigger_dist> <id> <x> <y> <w> <h> <frames> <anim_spd> <radius> <spd> <hp> <homing>\n");
  for (const auto& enemy : data.enemies) {
    fprintf(file, "ENEMY %ld %s %d %d %u %u %u %u %u %ld %d %d\n",
            static_cast<long>(enemy.trigger_distance), enemy.sprite_id.c_str(),
            enemy.x, enemy.y, enemy.width, enemy.height,
            enemy.num_frames, enemy.anim_speed, enemy.collider_radius,
            static_cast<long>(enemy.speed), enemy.hp, enemy.homing ? 1 : 0);
  }

  fclose(file);
  return true;
}

