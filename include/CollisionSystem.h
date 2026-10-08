#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <algorithm>
#include <nds.h>

#include "CircleColliderComponent.h"
#include "CollisionEvent.h"
#include "EventManager.h"
#include "System.h"
#include "TransformComponent.h"
#include "Vec2.h"

class CollisionSystem : public System {
 public:
  CollisionSystem(void) {
    this->require_component<CircleColliderComponent>();
    this->require_component<TransformComponent>();
  }

  void update(EventManager& event_manager) {
    auto entities = this->get_entities();

    for (auto i = entities.begin(); i != entities.end(); i++) {
      Entity a = *i;

      const auto& a_collider = a.get_component<CircleColliderComponent>();
      const auto& a_transform = a.get_component<TransformComponent>();

      for (auto j = std::next(i); j != entities.end(); j++) {
        Entity b = *j;

        const auto& b_collider = b.get_component<CircleColliderComponent>();
        const auto& b_transform = b.get_component<TransformComponent>();

        // Center calculation using Vec2f (fixed-point arithmetic)
        // a_collider.width / height should be fixed-point or converted via floatToFixed
        Vec2f a_center(
            a_transform.position.x + ((a_collider.width >> 1) * (a_transform.scale.x >> 12)),
            a_transform.position.y + ((a_collider.height >> 1) * (a_transform.scale.y >> 12))
        );

        Vec2f b_center(
            b_transform.position.x + ((b_collider.width >> 1) * (b_transform.scale.x >> 12)),
            b_transform.position.y + ((b_collider.height >> 1) * (b_transform.scale.y >> 12))
        );

        // Compute scaled radii in 20.12 fixed-point format:
        s32 max_scale_a = std::max(a_transform.scale.x, a_transform.scale.y);
        s32 max_scale_b = std::max(b_transform.scale.x, b_transform.scale.y);

        s32 a_radius = (a_collider.radius * max_scale_a) >> 12;
        s32 b_radius = (b_collider.radius * max_scale_b) >> 12;

        if (check_circular_collision(a_radius, b_radius, a_center, b_center)) {
          event_manager.emit_event<CollisionEvent>(a, b);
        }
      }
    }
  }

  bool check_circular_collision(s32 a_radius, s32 b_radius, Vec2f a_position, Vec2f b_position) {
    // Calculate delta distance in pixels:
    s32 dx = a_position.x - b_position.x;
    s32 dy = a_position.y - b_position.y;

    // Squared distance between centers:
    s32 dist_sq = (dx * dx) + (dy * dy);

    // Sum of radii squared:
    s32 radii_sum = a_radius + b_radius;
    s32 radii_sum_sq = radii_sum * radii_sum;

    return radii_sum_sq >= dist_sq;
  }
};

#endif  // COLLISIONSYSTEM_H
