#include "../include/SceneManager.h"

SceneManager::SceneManager(void) {}

SceneManager::~SceneManager(void) {
  if (this->current_scene != nullptr) {
    this->current_scene->on_exit();
    this->current_scene = nullptr;
  }

  for (auto& pair : this->scenes) {
    delete pair.second;
  }

  this->scenes.clear();
}

void SceneManager::add_scene(const std::string& name, Scene* scene) {
  auto it = this->scenes.find(name);

  if (it != this->scenes.end()) {
    if (this->current_scene == it->second) {
      this->current_scene->on_exit();
      this->current_scene = nullptr;
    }

    delete it->second;
  }

  this->scenes[name] = scene;
}

void SceneManager::switch_to(const std::string& name) {
  if (this->scenes.find(name) == this->scenes.end()) {
    return;
  }

  this->pending_scene_name = name;

  // If no scene is active yet, transition immediately:
  if (this->current_scene == nullptr) {
    this->apply_pending_switch();
  }
}

void SceneManager::apply_pending_switch(void) {
  if (this->pending_scene_name.empty()) {
    return;
  }

  auto it = this->scenes.find(this->pending_scene_name);
  this->pending_scene_name.clear();

  if (it == this->scenes.end()) {
    return;
  }

  if (this->current_scene != nullptr) {
    this->current_scene->on_exit();
  }

  this->current_scene = it->second;
  this->current_scene_name = it->first;
  this->current_scene->on_enter();
}

void SceneManager::process_input(u32 keys_pressed, u32 keys_released, u32 keys_held, touchPosition touch) {
  if (this->current_scene != nullptr) {
    this->current_scene->process_input(keys_pressed, keys_released, keys_held, touch);
  }
}

void SceneManager::update(float delta_time) {
  if (this->current_scene != nullptr) {
    this->current_scene->update(delta_time);
  }
}

void SceneManager::render(void) {
  if (this->current_scene != nullptr) {
    this->current_scene->render();
  }
}

Scene* SceneManager::get_current_scene(void) const {
  return this->current_scene;
}

const std::string& SceneManager::get_current_scene_name(void) const {
  return this->current_scene_name;
}

