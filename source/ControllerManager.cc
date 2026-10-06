#include "../include/ControllerManager.h"

ControllerManager::ControllerManager(void) {}

ControllerManager::~ControllerManager(void) {}

void ControllerManager::clear(void) {
  this->action_key_name.clear();
  this->keys_down.clear();
  this->mouse_button_name.clear();
  this->mouse_button_down.clear();
}

void ControllerManager::update(void) {
  uint32_t keys_held = keysHeld();

  // Refresh active states for all registered action keys
  for (auto& entry : this->keys_down) {
    entry.second = (keys_held & entry.first) != 0;
  }

  // Refresh active states for mouse/touch buttons
  for (auto& entry : this->mouse_button_down) {
    entry.second = (keys_held & entry.first) != 0;
  }
}

// Keyboard / Buttons:
void ControllerManager::add_action_key(const std::string& action, uint32_t key_mask) {
  this->action_key_name[action] = key_mask;
  this->keys_down[key_mask] = false;
}

bool ControllerManager::is_action_activated(const std::string& action) const {
  auto name_it = this->action_key_name.find(action);
  if (name_it == this->action_key_name.end()) {
    return false;
  }

  auto key_it = this->keys_down.find(name_it->second);
  if (key_it != this->keys_down.end()) {
    return key_it->second;
  }

  return false;
}

// Mouse / Touch Screen:
void ControllerManager::add_mouse_button(const std::string& name, uint32_t code) {
  this->mouse_button_name[name] = code;
  this->mouse_button_down[code] = false;
}

bool ControllerManager::is_mouse_button_down(const std::string& name) const {
  auto name_it = this->mouse_button_name.find(name);

  if (name_it == this->mouse_button_name.end()) {
    return false;
  }

  auto btn_it = this->mouse_button_down.find(name_it->second);

  if (btn_it != this->mouse_button_down.end()) {
    return btn_it->second;
  }

  return false;
}

void ControllerManager::set_mouse_position(uint16_t x, uint16_t y) {
  this->mouse_x_position = x;
  this->mouse_y_position = y;
}

std::tuple<uint16_t, uint16_t> ControllerManager::get_mouse_position(void) const {
  return {this->mouse_x_position, this->mouse_y_position};
}

void ControllerManager::set_mouse_button_down(uint32_t code) {
  this->mouse_button_down[code] = true;
}

void ControllerManager::set_mouse_button_up(uint32_t code) {
  this->mouse_button_down[code] = false;
}
