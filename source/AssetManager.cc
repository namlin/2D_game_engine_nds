#include "../include/AssetManager.h"

AssetManager::AssetManager(void) : next_available_slot(0) {}

AssetManager::~AssetManager(void) {
  this->clear_assets();
}

void AssetManager::clear_assets(void) {
  // Delete any active tiled backgrounds on Screen 0 layers 1..3:
  for (u8 layer = 1; layer <= 3; layer++) {
    if (NF_TILEDBG_LAYERS[0][layer].created) {
      NF_DeleteTiledBg(0, layer);
    }
  }

  // Unload tiled backgrounds from RAM:
  for (const auto& bg_id : this->loaded_tiled_bgs) {
    NF_UnloadTiledBg(bg_id.c_str());
  }
  this->loaded_tiled_bgs.clear();

  // Unload 2D/3D sprite graphics and palettes from RAM:
  for (const auto& [id, slot] : this->sprite_gfx_slots) {
    NF_UnloadSpriteGfx(slot);
    NF_UnloadSpritePal(slot);
  }

  // Unload 3D specific sprite VRAM and RAM slots:
  for (const auto& [id, slot] : this->sprite_gfx_ids) {
    if (NF_TEX256VRAM[slot].inuse) {
      NF_Free3dSpriteGfx(slot);
    }
    NF_UnloadSpriteGfx(slot);
  }

  for (const auto& [id, slot] : this->sprite_pal_ids) {
    NF_UnloadSpritePal(slot);
  }

  // Clear container maps and reset index counters:
  this->sprite_gfx_slots.clear();
  this->sprite_gfx_ids.clear();
  this->sprite_pal_ids.clear();
  this->next_available_slot = 0;
}

u16 AssetManager::get_sprite_id(const std::string& id) const {
  auto it = this->sprite_gfx_slots.find(id);

  if (it != this->sprite_gfx_slots.end()) {
    return it->second;
  }

  return 255;  // Invalid slot sentinel
}

void AssetManager::load_tiled_bg(const std::string& bg_id,
                                 const std::string& file_path, u16 width, u16 height) {
  if (this->loaded_tiled_bgs.find(bg_id) != this->loaded_tiled_bgs.end()) {
    return;
  }

  NF_LoadTiledBg(file_path.c_str(), bg_id.c_str(), width, height);
  this->loaded_tiled_bgs.insert(bg_id);
}

void AssetManager::unload_tiled_bg(const std::string& bg_id) {
  auto it = this->loaded_tiled_bgs.find(bg_id);
  if (it != this->loaded_tiled_bgs.end()) {
    NF_UnloadTiledBg(bg_id.c_str());
    this->loaded_tiled_bgs.erase(it);
  }
}

void AssetManager::load_3d_sprite(const std::string& sprite_id, const std::string& file_path,
                                  u16 width, u16 height, u16 gfx_slot, u16 pal_slot, bool keepframes) {
  if (this->sprite_gfx_ids.find(sprite_id) != this->sprite_gfx_ids.end()) {
    return;
  }

  // Load sprite graphics and palette from NitroFS into RAM:
  NF_LoadSpriteGfx(file_path.c_str(), gfx_slot, width, height);
  NF_LoadSpritePal(file_path.c_str(), pal_slot);

  // Transfer graphics and palette from RAM into 3D VRAM engine:
  NF_Vram3dSpriteGfx(gfx_slot, gfx_slot, keepframes);
  NF_Vram3dSpritePal(pal_slot, pal_slot);

  // Store slot mapping IDs:
  this->sprite_gfx_ids[sprite_id] = gfx_slot;
  this->sprite_pal_ids[sprite_id] = pal_slot;
}

u16 AssetManager::get_gfx_id(const std::string& sprite_id) const {
  auto it = this->sprite_gfx_ids.find(sprite_id);

  if (it != this->sprite_gfx_ids.end()) {
    return it->second;
  }

  return 255;
}

u16 AssetManager::get_pal_id(const std::string& sprite_id) const {
  auto it = this->sprite_pal_ids.find(sprite_id);

  if (it != this->sprite_pal_ids.end()) {
    return it->second;
  }

  return 255;
}
