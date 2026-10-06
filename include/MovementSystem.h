#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H

#include "../include/System.h"

// Components:
#include "../include/RigidBodyComponent.h"
#include "../include/TransformComponent.h"

class MovementSystem : public System {
 private:

 public:
  MovementSystem(void) {
    this->require_component<RigidBodyComponent>();
    this->require_component<TransformComponent>();
  }

  ~MovementSystem(void);

  void update(double delta_time) {
    for (auto entity : this->get_entities()) {
      const auto& rigid_body = entity.get_component<RigidBodyComponent>();
      auto& transform = entity.get_component<TransformComponent>();

      transform.position.x += rigid_body.velocity.x * delta_time;
      transform.position.y += rigid_body.velocity.y * delta_time;
    }
  }
};

#endif  // MOVEMENTSYSTEM_H
