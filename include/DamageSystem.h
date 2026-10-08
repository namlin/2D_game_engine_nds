#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H

#include "CircleColliderComponent.h"
#include "CollisionEvent.h"
#include "EventManager.h"
#include "HealthComponent.h"
#include "ProjectileComponent.h"
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
    Entity a = e.a;
    Entity b = e.b;

    // Check if 'a' is a projectile and 'b' has health:
    if (a.has_component<ProjectileComponent>() && b.has_component<HealthComponent>()) {
      apply_projectile_damage(a, b);
      return;
    }

    // Check if 'b' is a projectile and 'a' has health:
    if (b.has_component<ProjectileComponent>() && a.has_component<HealthComponent>()) {
      apply_projectile_damage(b, a);
      return;
    }

    // Fallback: If neither has ProjectileComponent, apply legacy behavior
    if (!a.has_component<ProjectileComponent>() && !b.has_component<ProjectileComponent>()) {
      a.delete_entity();
      b.delete_entity();
    }
  }

 private:
  void apply_projectile_damage(Entity projectile, Entity target) {
    const auto& proj = projectile.get_component<ProjectileComponent>();
    auto& health = target.get_component<HealthComponent>();

    health.health -= proj.damage;
    if (health.health < 0) {
      health.health = 0;
    }

    // Destroy the bullet upon impact:
    projectile.delete_entity();

    // Destroy target once health reaches 0:
    if (health.health <= 0) {
      target.delete_entity();
    }
  }
};

#endif  // DAMAGESYSTEM_H
