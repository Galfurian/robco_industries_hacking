#include "robsec/game.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "You need to provide a path to the dictionary.\n";
    std::cout << "Usage:\n";
    std::cout << "    " << argv[0] << " <dictionary>\n";
    return 1;
  }
  robsec::Game game(argv[1], 2, 4, 8);
  if (!game.initialize()) {
    return 1;
  }
  game.run();
  game.stop();
  return 0;
}
