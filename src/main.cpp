#include "robsec/game.hpp"

#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "You need to provide a path to the dictionary.\n";
        std::cout << "Usage:\n";
        std::cout << "    " << argv[0] << " <dictionary>\n";
        return 1;
    }
    robsec::Game game(argv[1], 3, 20, 12, 12, 4);
    if (!game.initialize()) {
        return 1;
    }
    bool state = game.run();
    game.stop();
    if (state) {
        printf("Terminal unlocked\n");
    } else {
        printf("Terminal locked\n");
        return 1;
    }
    return 0;
}
