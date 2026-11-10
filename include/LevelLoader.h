#ifndef LEVELLOADER_H
#define LEVELLOADER_H

#include <string>
#include "LevelData.h"

class LevelLoader {
 public:
  static bool load_from_file(const std::string& file_path, LevelData& out_data);
  static bool save_to_file(const std::string& file_path, const LevelData& data);
};

#endif  // LEVELLOADER_H

