#ifndef ENTITY_H
#define ENTITY_H

#include <cstddef>
#include <utility>

class Registry;

class Entity {
  friend class Registry;

 private:
  size_t id;
  Registry* registry = nullptr;

 public:
  explicit Entity(size_t id) : id(id) {}
  size_t get_id(void) const;
  void delete_entity(void);

  bool operator==(const Entity& other) const { return id == other.id; }
  bool operator!=(const Entity& other) const { return id != other.id; }
  bool operator>(const Entity& other) const { return id > other.id; }
  bool operator<(const Entity& other) const { return id < other.id; }

  template <typename TComponent, typename... TArgs>
  void add_component(TArgs&&... args);

  template <typename TComponent>
  void remove_component(void);

  template <typename TComponent>
  bool has_component(void) const;

  template <typename TComponent>
  TComponent& get_component(void) const;
};

#endif  // ENTITY_H
