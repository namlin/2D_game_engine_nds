#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <nds.h>
#include <nf_lib.h>

#include <string>
#include <unordered_map>

class AssetManager {
 private:
  // Maps string IDs (e.g., "player") to NFlib Sprite Graphics Slot IDs (0-255):
  std::unordered_map<std::string, u16> sprite_gfx_slots;

  // Tracks the current available slot index:
  u16 next_available_slot;

 public:
  AssetManager(void);
  ~AssetManager(void);

  void clear_assets(void);

  /**
    * Loads a sprite graphics asset into VRAM/RAM using NFlib.
    * @param file_path File name inside NitroFS (e.g., "sprite/player" for player.img/player.pal)
    * @param texture_id Unique string identifier for this asset
    * @param width Width of the sprite in pixels (e.g., 16, 32, 64)
    * @param height Height of the sprite in pixels
    */
  void add_texture(const std::string& texture_id, const char* file_path, u16 width, u16 height);

  /**
    * Returns the NFlib graphics slot ID associated with the text ID.
    * Returns 255 (invalid) if not found.
    */
  u16 get_texture_id(const std::string& id) const;
};

#endif  // ASSETMANAGER_H
