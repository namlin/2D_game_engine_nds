#ifndef CLICKEVENT_H
#define CLICKEVENT_H

#include "../include/Event.h"

class ClickEvent : public Event {
 public:
  uint8_t button_code = 0;
  size_t x_position = 0;
  size_t y_position = 0;

  ClickEvent(uint8_t button_code, size_t x_position, size_t y_position) {
    this->button_code = button_code;
    this->x_position = x_position;
    this->y_position = y_position;
  }
};

#endif  // CLICKEVENT_H
