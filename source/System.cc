#include "../include/System.h"

void System::add_entity(Entity entity) {
  this->entities.push_back(entity);
}

void System::remove_entity(Entity entity) {
  auto it = std::remove_if(this->entities.begin(), this->entities.end(),
                           [&entity](Entity other) {return entity == other;});

  this->entities.erase(it, this->entities.end());
}

std::vector<Entity> System::get_entities(void) const {
  return entities;
}

std::bitset<MAX_COMPONENTS>& System::get_component_signature(void) {
  return this->signature;
}
