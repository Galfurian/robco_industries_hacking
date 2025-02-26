#include "robsec/game.hpp"

#include <iostream>

#include <cmdlp/parser.hpp>
#include <ncurses.h>

int main(int argc, char *argv[])
{
    cmdlp::Parser parser(argc, argv);
    parser.addOption("-d", "--dictionary", "The path to the dictionary.", true, "");
    parser.addOption("-p", "--pannels", "The number of pannels.", false, 3);
    parser.addOption("-r", "--rows", "The number of rows.", false, 20);
    parser.addOption("-c", "--columns", "The number of columns.", false, 12);
    parser.addOption("-w", "--words", "The number of words.", false, 12);
    parser.addOption("-a", "--attemps", "The number of attemps.", false, 4);
    parser.parseOptions();

    robsec::Game game(
        parser.getOption<std::string>("-d"),
        parser.getOption<unsigned>("-p"),
        parser.getOption<unsigned>("-r"),
        parser.getOption<unsigned>("-c"),
        parser.getOption<unsigned>("-w"),
        parser.getOption<int>("-a"));
    if (!game.initialize()) {
        game.print_log();
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
