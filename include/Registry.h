#ifndef REGISTRY_H
#define REGISTRY_H

#include <bitset>
#include <deque>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Component.h"
#include "Constants.h"
#include "Entity.h"
#include "Pool.h"
#include "System.h"

class System;

class Registry {
 public:
  Registry(void);
  ~Registry(void);

  void update(void);

  // Entity Management:
  Entity create_entity(void);
  void delete_entity(Entity entity);

  // Component Management:
  template <typename TComponent, typename... TArgs>
  void add_component(Entity entity, TArgs&&... args);

  template <typename TComponent>
  void remove_component(Entity entity);

  template <typename TComponent>
  bool has_component(Entity entity) const;

  template <typename TComponent>
  TComponent& get_component(Entity entity);

  // System Management:
  template <typename TSystem, typename... TArgs>
  void add_system(TArgs&&... args);

  template <typename TSystem>
  void remove_system(void);

  template <typename TSystem>
  bool has_system(void) const;

  template <typename TSystem>
  TSystem& get_system(void) const;

  void add_entity_to_systems(Entity entity);
  void remove_entity_from_systems(Entity entity);

 private:
  size_t total_entities = 0;
  std::vector<IPool*> entries;
  std::vector<std::bitset<MAX_COMPONENTS>> entity_component_signatures;
  std::unordered_map<std::type_index, System*> systems;
  std::deque<size_t> free_IDs;

  std::set<Entity> entities_to_add;
  std::set<Entity> entities_to_remove;
};

template <typename TComponent, typename... TArgs>
void Registry::add_component(Entity entity, TArgs&&... args) {
  const size_t component_id = Component<TComponent>::get_id();
  const size_t entity_id = entity.get_id();

  if (component_id >= this->entries.size()) {
    this->entries.resize(component_id + 10, nullptr);
  }

  if (!this->entries[component_id]) {
    auto* new_component = new Pool<TComponent>();
    this->entries[component_id] = new_component;
  }

  if (entity_id >= this->entity_component_signatures.size()) {
    this->entity_component_signatures.resize(entity_id + 100);
  }

  TComponent component(std::forward<TArgs>(args)...);

  auto* pool = static_cast<Pool<TComponent>*>(this->entries[component_id]);
  pool->set(entity_id, component);
  this->entity_component_signatures[entity_id].set(component_id);
}

template <typename TComponent>
void Registry::remove_component(Entity entity) {
  const size_t component_id = Component<TComponent>::get_id();
  const size_t entity_id = entity.get_id();

  if (entity_id < this->entity_component_signatures.size()) {
    this->entity_component_signatures[entity_id].set(component_id, false);
  }
}

template <typename TComponent>
bool Registry::has_component(Entity entity) const {
  const size_t component_id = Component<TComponent>::get_id();
  const size_t entity_id = entity.get_id();

  if (entity_id >= this->entity_component_signatures.size()) {
    return false;
  }

  return this->entity_component_signatures[entity_id].test(component_id);
}

template <typename TComponent>
TComponent& Registry::get_component(Entity entity) {
  const size_t component_id = Component<TComponent>::get_id();
  const size_t entity_id = entity.get_id();

  auto* component_pool = static_cast<Pool<TComponent>*>(this->entries[component_id]);
  return component_pool->get(entity_id);
}

template <typename TSystem, typename... TArgs>
void Registry::add_system(TArgs&&... args) {
  auto* system = new TSystem(std::forward<TArgs>(args)...);
  this->systems.insert(std::make_pair(std::type_index(typeid(TSystem)), system));
}

template <typename TSystem>
void Registry::remove_system(void) {
  auto system = this->systems.find(std::type_index(typeid(TSystem)));
  this->systems.erase(system);
}

template <typename TSystem>
bool Registry::has_system(void) const {
  return this->systems.find(std::type_index(typeid(TSystem))) != this->systems.end();
}

template <typename TSystem>
TSystem& Registry::get_system(void) const {
  auto system = this->systems.find(std::type_index(typeid(TSystem)));

  if (system == this->systems.end()) {
    throw std::runtime_error("System not found");
  }

  return *static_cast<TSystem*>(system->second);
}

template <typename TComponent, typename... TArgs>
void Entity::add_component(TArgs&&... args) {
  this->registry->template add_component<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
void Entity::remove_component(void) {
  this->registry->template remove_component<TComponent>(*this);
}

template <typename TComponent>
bool Entity::has_component(void) const {
  return this->registry->template has_component<TComponent>(*this);
}

template <typename TComponent>
TComponent& Entity::get_component(void) const {
  return this->registry->template get_component<TComponent>(*this);
}

#endif  // REGISTRY_H
