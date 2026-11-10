#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <nds.h>
#include <string>
#include <unordered_map>

#include "Scene.h"

class SceneManager {
 private:
  std::unordered_map<std::string, Scene*> scenes;
  Scene* current_scene = nullptr;
  std::string current_scene_name = "";
  std::string pending_scene_name = "";

 public:
  SceneManager(void);
  ~SceneManager(void);

  void add_scene(const std::string& name, Scene* scene);
  void switch_to(const std::string& name);
  void apply_pending_switch(void);

  void process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch);
  void update(float delta_time);
  void render(void);

  Scene* get_current_scene(void) const;
  const std::string& get_current_scene_name(void) const;
};

#endif  // SCENEMANAGER_H

