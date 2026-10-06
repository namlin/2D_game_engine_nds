#ifndef RIGIDBODYCOMPONENT_H
#define RIGIDBODYCOMPONENT_H

#include "Vec2.h"

struct RigidBodyComponent {
    Vec2f velocity;

    RigidBodyComponent(Vec2f velocity = Vec2f(0, 0)) : velocity(velocity) {}
};

#endif // RIGIDBODYCOMPONENT_H
