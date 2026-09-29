#ifndef COMPONENT_H
#define COMPONENT_H

#include <cstddef>

struct IComponent {
 protected:
  static size_t get_next_id(void) {
    static size_t next_id = 0;
    return next_id++;
  }
};

template <typename TComponent>
class Component : public IComponent {
 public:
  static size_t get_id(void) {
    static size_t id = IComponent::get_next_id();
    return id;
  }
};

#endif  // COMPONENT_H
