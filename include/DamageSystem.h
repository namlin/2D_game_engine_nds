#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H

#include "CircleColliderComponent.h"
#include "CollisionEvent.h"
#include "EventManager.h"
#include "System.h"

class DamageSystem : public System {
 public:
  DamageSystem(void) {
    this->require_component<CircleColliderComponent>();
  }

  void subscribe_to_collision_event(EventManager& event_manager) {
    event_manager.subscribe_to_event<CollisionEvent, DamageSystem>(this, &DamageSystem::on_collision);
  }

  void on_collision(CollisionEvent& e) {
    e.a.delete_entity();
    e.b.delete_entity();
  }
};

#endif  // DAMAGESYSTEM_H
