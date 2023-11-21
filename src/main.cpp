#include "robsec/game.hpp"

#include <iostream>
#include <cmdlp/option_parser.hpp>

int main(int argc, char *argv[])
{
    cmdlp::OptionParser parser(argc, argv);
    parser.addOption("-d", "--dictionary", "The path to the dictionary.", "", true);
    parser.addOption("-p", "--pannels", "The number of pannels.", 3, false);
    parser.addOption("-r", "--rows", "The number of rows.", 20, false);
    parser.addOption("-c", "--columns", "The number of columns.", 12, false);
    parser.addOption("-w", "--words", "The number of words.", 12, false);
    parser.addOption("-a", "--attemps", "The number of attemps.", 4, false);
    parser.parseOptions();

    robsec::Game game(
        parser.getOption<std::string>("-d"),
        parser.getOption<unsigned>("-p"),
        parser.getOption<unsigned>("-r"),
        parser.getOption<unsigned>("-c"),
        parser.getOption<unsigned>("-w"),
        parser.getOption<int>("-a"));
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
