#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H

struct HealthComponent {
  int health;
  int max_health;

  HealthComponent(int health = 100, int max_health = 100)
      : health(health), max_health(max_health) {}
};

#endif  // HEALTHCOMPONENT_H
