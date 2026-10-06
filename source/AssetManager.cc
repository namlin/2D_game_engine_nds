#include "../include/AssetManager.h"

AssetManager::AssetManager(void) : next_available_slot(0) {}

AssetManager::~AssetManager(void) {
  this->clear_assets();
}

void AssetManager::clear_assets(void) {
  for (const auto& [id, slot] : this->sprite_gfx_slots) {
    // Unload sprite graphics from NFlib RAM storage
    NF_UnloadSpriteGfx(slot);
  }

  this->sprite_gfx_slots.clear();
  this->next_available_slot = 0;
}

void AssetManager::add_texture(const std::string& texture_id, const char* file_path, u16 width, u16 height) {
  // Check if slot limit is reached
  if (this->next_available_slot >= 256) {
    // std::cerr << "Error: Reached maximum NFlib sprite graphics limit (256).\n";
    return;
  }

  u16 current_slot = this->next_available_slot;

  // NF_LoadSpriteGfx loads sprite .img and .pal files from nitroFS
  // Parameters: path, gfx_slot, width, height
  NF_LoadSpriteGfx(file_path, current_slot, width, height);

  // Keep track of the loaded slot
  this->sprite_gfx_slots.emplace(texture_id, current_slot);

  // Increment slot for the next asset
  this->next_available_slot++;
}

u16 AssetManager::get_texture_id(const std::string& id) const {
  auto it = this->sprite_gfx_slots.find(id);

  if (it != this->sprite_gfx_slots.end()) {
    return it->second;  // Returns NFlib slot number.
  }

  return 255;  // 255 is typically used as an invalid slot sentinel in NFlib.
}
