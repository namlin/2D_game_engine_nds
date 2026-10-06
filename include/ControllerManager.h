#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <nds.h>
#include <nf_lib.h>

#include <map>
#include <string>
#include <tuple>

class ControllerManager {
 private:
  std::map<std::string, uint32_t> action_key_name;
  std::map<uint32_t, bool> keys_down;

  std::map<std::string, uint32_t> mouse_button_name;
  std::map<uint32_t, bool> mouse_button_down;

  uint16_t mouse_x_position = 0;
  uint16_t mouse_y_position = 0;

 public:
  ControllerManager(void);
  ~ControllerManager(void);

  void clear(void);

  // Poll input every frame:
  void update(void);

  // Keyboard / NDS Buttons:
  void add_action_key(const std::string& action, uint32_t key_mask);
  bool is_action_activated(const std::string& action) const;

  // Touch Screen:
  void add_mouse_button(const std::string& name, uint32_t code);
  bool is_mouse_button_down(const std::string& name) const;

  void set_mouse_position(uint16_t x, uint16_t y);
  std::tuple<uint16_t, uint16_t> get_mouse_position(void) const;

  void set_mouse_button_down(uint32_t code);
  void set_mouse_button_up(uint32_t code);
};

#endif  // CONTROLLERMANAGER_H
