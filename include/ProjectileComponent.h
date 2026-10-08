#ifndef PROJECTILECOMPONENT_H
#define PROJECTILECOMPONENT_H

struct ProjectileComponent {
  int damage;
  bool is_friendly;

  ProjectileComponent(int damage = 20, bool is_friendly = true)
      : damage(damage), is_friendly(is_friendly) {}
};

#endif  // PROJECTILECOMPONENT_H
