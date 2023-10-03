/// @file game.cpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#include "robsec/game.hpp"

#include <algorithm>
#include <cstring>
#include <curses.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

template <typename T> static inline T random_number(T min, T max) {
  // the random device that will seed the generator
  std::random_device seeder;
  // then make a mersenne twister engine
  std::mt19937 engine(seeder());
  // then the easy part... the distribution
  std::uniform_int_distribution<T> dist(min, max);
  return dist(engine);
}

static inline std::string generate_garbage_string(std::size_t width) {
  static const char *garbage = ",|\\!@#$%^&*-_+=.:;?,/";
  std::string s;
  for (std::size_t i = 0; i < width; ++i) {
    s.push_back(
        garbage[random_number<std::size_t>(0, std::strlen(garbage) - 1)]);
  }
  return s;
}

namespace robsec {

screen_coord game_coord::to_screen_coord(std::size_t panel_size) const {
  return screen_coord(
      (6 + 1) * (panel + 1) + (2 + panel_size) * (panel) + column, 5 + row);
}

game_coord screen_coord::to_game_coord(std::size_t panel_size) const {
  return game_coord((x - 7) / (6 + 1 + panel_size + 2), y - 5,
                    x % (6 + 1 + panel_size + 2) - 7);
}

Game::Game(std::string _dictionary_path, std::size_t _panels, std::size_t _rows,
           std::size_t _panel_size)
    : dictionary_path(_dictionary_path), start_address(0), panels(_panels),
      rows(_rows), panel_size(_panel_size), position({0, 0, 0}) {
  //
  std::srand(std::time(NULL));
  // Compute the starting address.
  start_address =
      random_number<std::size_t>(0xA000, 0xFFFF - rows * panels * panel_size);
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

bool Game::initialize() {

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
    std::string string =
        dictionary[random_number<std::size_t>(0, dictionary.size() - 1)];
    // Prepare the word object.
    Word word(0, 0, string);
    // Check if we already selected this word.
    if (std::find(words.begin(), words.end(), word) == words.end()) {
      // Find a random place.
      if (this->find_unoccupied_space_for_word(word)) {
        total_words--;
        // Place the word.
        content[word.panel].replace(word.start, word.length, word.word.c_str());
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

void Game::stop() { endwin(); }

void Game::run() {
  mousemask(ALL_MOUSE_EVENTS, NULL);

  for (int ch = getch(); ch != 'q'; ch = getch()) {
    // Parse the input.
    this->parse_input(ch);
    // Update the scene.
    this->update();
    // Move the cursor.
    this->move_cursor_to(position.x, position.y, position.panel);
    refresh();
  }
}

void Game::update() {
  // Move the cursor at the beginning.
  wmove(stdscr, 0, 0);
  // Print the header.
  printw("ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL\n");
  printw("ENTER PASSWORD NOW\n");
  printw("\n");
  printw("4 ATTEMP(S) LEFT : # # # #\n");
  printw("\n");
  clrtoeol();
  refresh();
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
  printw("\n--DEBUG--\n");
  for (std::size_t i = 0; i < words.size(); ++i) {
    if (words[i].is_selected(position.panel,
                             position.y * panel_size + position.x)) {
      printw("[X] %s\n", words[i].word.c_str());
    } else {
      printw("[ ] %s\n", words[i].word.c_str());
    }
  }
}

void Game::print_char(std::size_t &panel, std::size_t &x, std::size_t &y,
                      char c) {}

void Game::parse_input(int key) {
  // Start from the current positions.
  std::size_t y = position.y;
  std::size_t x = position.x;
  std::size_t panel = position.panel;
  // Check if it was a mouse click.
  if (!this->parse_mouse_position(key, x, y, panel)) {
    // Check if it was a valid key.
    if (!this->parse_key_position(key, x, y, panel)) {
      return;
    }
  }
  // Update the internal position.
  position.x = x;
  position.y = y;
  position.panel = panel;
}

bool Game::parse_mouse_position(int key, std::size_t &x, std::size_t &y,
                                std::size_t &panel) const {
  if (key == KEY_MOUSE) {
    MEVENT event;
    if (getmouse(&event) == OK) {
      y = event.y - 5;
      x = event.x % (6 + 1 + panel_size + 2) - 7;
      panel = (event.x - 7) / (6 + 1 + panel_size + 2);
      if ((y < rows) && (x < panel_size)) {
        return true;
      }
    }
  }
  return false;
}

bool Game::parse_key_position(int key, std::size_t &x, std::size_t &y,
                              std::size_t &panel) const {
  if ((key == KEY_UP) && (y > 0)) {
    y = y - 1;
  } else if ((key == KEY_DOWN) && (y < rows - 1)) {
    y = y + 1;
  } else if (key == KEY_LEFT) {
    if (x > 0) {
      x = x - 1;
    } else if (panel > 0) {
      panel = panel - 1;
      x = panel_size - 1;
    }
  } else if (key == KEY_RIGHT) {
    if (x < panel_size - 1) {
      x = x + 1;
    } else if (panel < panels - 1) {
      panel = panel + 1;
      x = 0;
    }
  } else {
    return false;
  }
  return true;
}

void Game::move_cursor_to(std::size_t x, std::size_t y,
                          std::size_t panel) const {
  if ((y < rows) && (x < panel_size) && (panel < panels)) {
    wmove(stdscr, 5 + y,
          (6 + 1) * (panel + 1) + (2 + panel_size) * (panel) + x);
  }
}

bool Game::find_unoccupied_space_for_word(Word &word) const {
  bool overlaps;
  for (std::size_t round = 0; round < 20; ++round) {
    // Place the word.
    word.panel = random_number<std::size_t>(0, panels - 1);
    word.start = random_number<std::size_t>(0, rows * panel_size - word.length);
    word.end = word.start + word.length;
    // Check if it overlaps with another word.
    overlaps = false;
    for (std::size_t i = 0; i < words.size(); ++i) {
      if (word.overlap(words[i])) {
        overlaps = true;
        break;
      }
    }
    if (!overlaps) {
      return true;
    }
  }
  word.panel = 0;
  word.start = 0;
  word.end = 0;
  return false;
}

std::size_t Game::compute_address(std::size_t row, std::size_t panel) const {
  return start_address + row * panel_size + panel * rows * panel_size;
}

} // namespace robsec
