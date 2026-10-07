#include <filesystem.h>
#include <nds.h>
#include <nf_lib.h>

#include "../include/Game.h"

int main(int argc, char **argv) {
  Game* game = Game::get_instance();
  game->init();
  game->setup();
  game->run();
  game->destroy();

  return 0;
}
