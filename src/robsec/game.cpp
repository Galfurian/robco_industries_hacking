/// @file game.cpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#include "robsec/game.hpp"

#include <curses.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#define ADDRESS_LEN 6
#define HEADER                                  \
    "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL\n" \
    "ENTER PASSWORD NOW\n"                      \
    "\n"
#define HEADER_LEN (3 + 2)

template <typename T>
static inline T random_number(T min, T max)
{
    // the random device that will seed the generator
    std::random_device seeder;
    // then make a mersenne twister engine
    std::mt19937 engine(seeder());
    // then the easy part... the distribution
    std::uniform_int_distribution<T> dist(min, max);
    return dist(engine);
}

static inline std::string generate_garbage_string(std::size_t width)
{
    static const char *garbage = ",|\\!@#$%^&*-_+=.:;?,/";
    std::string s;
    for (std::size_t i = 0; i < width; ++i) {
        s.push_back(garbage[random_number<std::size_t>(0, std::strlen(garbage) - 1)]);
    }
    return s;
}

namespace robsec
{

Game::Game(std::string _dictionary_path, std::size_t _panels, std::size_t _rows, std::size_t _panel_size)
    : dictionary_path(_dictionary_path),
      start_address(0),
      panels(_panels),
      rows(_rows),
      panel_size(_panel_size),
      attempts_max(4),
      attempts(attempts_max),
      position({ 0, 0, 0 })
{
    //
    std::srand(static_cast<unsigned>(std::time(NULL)));
    // Compute the starting address.
    start_address = random_number<std::size_t>(0xA000, 0xFFFF - rows * panels * panel_size);
    //
    initscr();
    clear();
    noecho();
    cbreak();
    keypad(stdscr, true);
    // Call function to use color.
    start_color();
    // Use default colors.
    use_default_colors();
    // Initialize the panel content.
    content.resize(panels);
    // Fill the panel content.
    for (std::size_t c = 0; c < panels; ++c) {
        content[c] = generate_garbage_string(rows * panel_size);
    }
}

bool Game::initialize()
{
    // ==========================================================================
    // Load the dictionary.
    std::ifstream file(dictionary_path);
    if (!file.is_open()) {
        return false;
    }
    std::string word;
    while (file >> word) {
        // displaying content
        dictionary.emplace_back(word);
    }
    file.close();

    // ==========================================================================
    // Select and place the words.
    int total_words = 3, round = 10;
    while (total_words && round) {
        // Get a random word.
        std::string string = dictionary[random_number<std::size_t>(0, dictionary.size() - 1)];
        // Prepare the word object.
        Word word(0, 0, string);
        // Check if we already selected this word.
        if (std::find(words.begin(), words.end(), word) == words.end()) {
            // Find a random place.
            if (this->find_unoccupied_space_for_word(word)) {
                total_words--;
                // Place the word.
                content[word.panel].replace(word.start, word.length, word.string.c_str());
                words.emplace_back(word);
                continue;
            }
        }
        round--;
    }
    if (round == 0) {
        printw("Failed to initialize the system.\n");
        return false;
    }
    this->update();
    this->move_cursor_to(0, 0, 0);
    return true;
}

void Game::stop()
{
    endwin();
}

void Game::run()
{
    mousemask(ALL_MOUSE_EVENTS, NULL);

    for (int ch = getch(); ch != 'q'; ch = getch()) {
        // Parse the input.
        this->parse_input(ch);
        // Update the scene.
        this->update();
        // Move the cursor.
        this->move_cursor_to(position);
        refresh();
    }
}

void Game::update()
{
    // Move the cursor at the beginning.
    wmove(stdscr, 0, 0);
    // Print the header.
    printw(HEADER);
    // Print attempts.
    printw("%lu ATTEMP(S) LEFT :", attempts);
    for (std::size_t i = 0; i < attempts; ++i) {
        printw(" #");
    }
    printw("\n\n");
    std::size_t address;
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < panels; ++c) {
            // Compute the address.
            address = this->compute_address(r, c);
            // Print the address.
            printw("0x%04zX ", address);
            // Print the content.
            printw("%s", content[c].substr(r * panel_size, panel_size).c_str());
            printw("  ");
        }
        printw("\n");
    }
    printw("\n");
    printw("Press 'q' to exit\n");
    for (const auto &word : words) {
        if (word.is_selected(position.panel, position.row * panel_size + position.column)) {
            attron(A_REVERSE);
            for (std::size_t j = 0; j < word.length; ++j) {
                mvaddch(word.coordinates[j].y, word.coordinates[j].x, word.string[j]);
            }
            attroff(A_REVERSE);
            break;
        }
    }
}

void Game::parse_input(int key)
{
    // Check if it was a mouse click.
    if (!this->parse_mouse_position(key, position)) {
        // Check if it was a valid key.
        if (!this->parse_key_position(key, position)) {
            return;
        }
    }
}

bool Game::parse_mouse_position(int key, GameLocation &location) const
{
    if (key == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK) {
            ScreenLocation coord(static_cast<std::size_t>(event.x), static_cast<std::size_t>(event.y));
            GameLocation new_location = this->to_game_location(coord);
            if ((new_location.panel < panels) && (new_location.row < rows) && (new_location.column < panel_size)) {
                location = new_location;
                return true;
            }
        }
    }
    return false;
}

bool Game::parse_key_position(int key, GameLocation &location) const
{
    if ((key == KEY_UP) && (location.row > 0)) {
        --location.row;
    } else if ((key == KEY_DOWN) && (location.row < rows - 1)) {
        ++location.row;
    } else if (key == KEY_LEFT) {
        if (location.column > 0) {
            --location.column;
        } else if (location.panel > 0) {
            --location.column = panel_size - 1;
            --location.panel;
        }
    } else if (key == KEY_RIGHT) {
        if (location.column < panel_size - 1) {
            ++location.column;
        } else if (location.panel < panels - 1) {
            location.column = 0;
            ++location.panel;
        }
    } else {
        return false;
    }
    return true;
}

bool Game::find_unoccupied_space_for_word(Word &word) const
{
    bool overlaps;
    for (std::size_t round = 0; round < 20; ++round) {
        // Place the word.
        word.panel = random_number<std::size_t>(0, panels - 1);
        word.start = random_number<std::size_t>(0, rows * panel_size - word.length);
        word.end   = word.start + word.length;
        // Check if it overlaps with another word.
        overlaps = false;
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (word.overlap(words[i])) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            for (std::size_t i = 0; i < word.length; ++i) {
                word.coordinates.emplace_back(this->linear_to_screen_location(word.panel, word.start + i));
            }
            return true;
        }
    }
    word.panel = 0;
    word.start = 0;
    word.end   = 0;
    return false;
}

std::size_t Game::compute_address(std::size_t row, std::size_t panel) const
{
    return start_address + row * panel_size + panel * rows * panel_size;
}

ScreenLocation Game::to_screen_location(const GameLocation &location) const
{
    return ScreenLocation((ADDRESS_LEN + 1) * (location.panel + 1) + (2 + panel_size) * location.panel + location.column,
                          HEADER_LEN + location.row);
}

GameLocation Game::to_game_location(const ScreenLocation &location) const
{
    return GameLocation((location.x - ADDRESS_LEN - 1) / (ADDRESS_LEN + 1 + panel_size + 2),
                        location.x % (ADDRESS_LEN + 1 + panel_size + 2) - ADDRESS_LEN - 1, location.y - HEADER_LEN);
}

GameLocation Game::linear_to_game_location(std::size_t panel, std::size_t position) const
{
    return GameLocation(panel, position % panel_size, position / panel_size);
}

ScreenLocation Game::linear_to_screen_location(std::size_t panel, std::size_t position) const
{
    return this->to_screen_location(this->linear_to_game_location(panel, position));
}

void Game::move_cursor_to(std::size_t panel, std::size_t column, std::size_t row) const
{
    this->move_cursor_to(GameLocation(panel, column, row));
}

void Game::move_cursor_to(const GameLocation &location) const
{
    this->move_cursor_to(this->to_screen_location(location));
}

void Game::move_cursor_to(const ScreenLocation &location) const
{
    this->move_cursor_to(static_cast<int>(location.x), static_cast<int>(location.y));
}

void Game::move_cursor_to(int x, int y) const
{
    if ((y < getmaxy(stdscr)) && (x < getmaxx(stdscr))) {
        wmove(stdscr, y, x);
    }
}

} // namespace robsec
