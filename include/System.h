#ifndef SYSTEM_H
#define SYSTEM_H

#include <algorithm>
#include <bitset>
#include <vector>

#include "Component.h"
#include "Constants.h"
#include "Entity.h"

// Abstract class for all the systems:
class System {
 private:
  std::bitset<MAX_COMPONENTS> signature;
  std::vector<Entity> entities;

 public:
  System(void) = default;
  ~System(void) = default;

  void add_entity(Entity entity);
  void remove_entity(Entity entity);

  std::vector<Entity> get_entities(void) const;

  std::bitset<MAX_COMPONENTS>& get_component_signature(void);

  template <typename TComponent>
  void require_component(void);
};

template <typename TComponent>
void System::require_component(void) {
  const size_t component_id = Component<TComponent>::get_id();
  this->signature.set(component_id);
}

#endif  // SYSTEM_H
