#include "../include/Entity.h"
#include "../include/Registry.h"

size_t Entity::get_id(void) const {
  return this->id;
}

void Entity::delete_entity(void) {
  this->registry->delete_entity(*this);
}

bool Entity::is_alive(void) const {
  if (this->registry == nullptr) return false;
  return !this->registry->is_entity_to_remove(*this);
}
