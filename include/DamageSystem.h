#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H

#include "CircleColliderComponent.h"
#include "CollisionEvent.h"
#include "EnemyComponent.h"
#include "EventManager.h"
#include "HealthComponent.h"
#include "ProjectileComponent.h"
#include "PropComponent.h"
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

    // Check if 'a' is a projectile:
    if (a.has_component<ProjectileComponent>()) {
      handle_projectile_collision(a, b);
      return;
    }

    // Check if 'b' is a projectile:
    if (b.has_component<ProjectileComponent>()) {
      handle_projectile_collision(b, a);
      return;
    }

    // Ignore collisions between two enemies or two props, or enemy vs prop:
    bool a_is_enemy_or_prop = a.has_component<EnemyComponent>() || a.has_component<PropComponent>();
    bool b_is_enemy_or_prop = b.has_component<EnemyComponent>() || b.has_component<PropComponent>();

    if (a_is_enemy_or_prop && b_is_enemy_or_prop) {
      return;
    }

    // Player vs Enemy or Prop:
    if (!a.has_component<PropComponent>() || a.has_component<HealthComponent>()) {
      a.delete_entity();
    }

    if (!b.has_component<PropComponent>() || b.has_component<HealthComponent>()) {
      b.delete_entity();
    }
  }

 private:
  void handle_projectile_collision(Entity projectile, Entity target) {
    if (target.has_component<ProjectileComponent>()) {
      return;
    }

    const auto& proj = projectile.get_component<ProjectileComponent>();

    // Projectile hitting a Prop / Furniture:
    if (target.has_component<PropComponent>()) {
      const auto& prop = target.get_component<PropComponent>();
      if (prop.blocks_projectiles) {
        if (prop.is_destructible && target.has_component<HealthComponent>()) {
          apply_projectile_damage(projectile, target);
        } else {
          projectile.delete_entity();
        }
      }
      return;
    }

    if (!target.has_component<HealthComponent>()) {
      return;
    }

    if (proj.is_friendly && target.has_component<EnemyComponent>()) {
      apply_projectile_damage(projectile, target);
      return;
    }

    if (!proj.is_friendly && !target.has_component<EnemyComponent>()) {
      apply_projectile_damage(projectile, target);
      return;
    }
  }

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
