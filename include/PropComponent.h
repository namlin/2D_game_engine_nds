#ifndef PROPCOMPONENT_H
#define PROPCOMPONENT_H

#include <nds.h>

struct PropComponent {
  bool is_destructible;
  bool blocks_projectiles;

  PropComponent(bool is_destructible = false, bool blocks_projectiles = true)
      : is_destructible(is_destructible), blocks_projectiles(blocks_projectiles) {}
};

#endif  // PROPCOMPONENT_H

