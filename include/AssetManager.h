#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <nds.h>
#include <nf_lib.h>

#include <string>
#include <unordered_map>

class AssetManager {
 private:
  std::unordered_map<std::string, u16> sprite_gfx_slots;
  std::unordered_map<std::string, u16> sprite_gfx_ids;
  std::unordered_map<std::string, u16> sprite_pal_ids;
  u16 next_available_slot;

 public:
  AssetManager(void);
  ~AssetManager(void);

  void clear_assets(void);
  void add_sprite(const std::string& texture_id, const char* file_path, u16 width, u16 height);
  u16 get_sprite_id(const std::string& id) const;

  void load_tiled_bg(const std::string& bg_id, const std::string& file_path, u16 width, u16 height);
  void load_3d_sprite(const std::string& sprite_id, const std::string& file_path,
                      u16 width, u16 height, u16 gfx_slot, u16 pal_slot);

  u16 get_gfx_id(const std::string& sprite_id) const;
  u16 get_pal_id(const std::string& sprite_id) const;
};

#endif // ASSETMANAGER_H
