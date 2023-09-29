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

static inline int random_number(int min, int max) {
  // the random device that will seed the generator
  std::random_device seeder;
  // then make a mersenne twister engine
  std::mt19937 engine(seeder());
  // then the easy part... the distribution
  std::uniform_int_distribution<int> dist(min, max);
  return dist(engine);
}

static inline std::string generate_garbage_string(int width) {
  static const char *garbage = ",|\\!@#$%^&*-_+=.:;?,/";
  std::string s;
  for (int i = 0; i < width; ++i) {
    s.push_back(garbage[random_number(0, std::strlen(garbage) - 1)]);
  }
  return s;
}

namespace robsec {

Game::Game(std::string _dictionary_path, int _columns, int _rows,
           int _column_size)
    : dictionary_path(_dictionary_path), start_address(0), columns(_columns),
      rows(_rows), column_size(_column_size), position({0, 0, 0}) {
  //
  std::srand(std::time(NULL));
  // Compute the starting address.
  start_address = random_number(0xA000, 0xFFFF - rows * columns * column_size);
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
  // Initialize the column content.
  content.resize(columns);
  // Fill the column content.
  for (int c = 0; c < columns; ++c) {
    content[c] = generate_garbage_string(rows * column_size);
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
    std::string string = dictionary[random_number(0, dictionary.size() - 1)];
    // Prepare the word object.
    Word word(0, 0, string);
    // Check if we already selected this word.
    if (std::find(words.begin(), words.end(), word) == words.end()) {
      // Find a random place.
      if (this->find_unoccupied_space_for_word(word)) {
        total_words--;
        // Place the word.
        content[word.column].replace(word.start, word.length,
                                     word.word.c_str());
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

  // ==========================================================================
  // Print the header.
  printw("ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL\n");
  printw("ENTER PASSWORD NOW\n");
  printw("\n");
  printw("4 ATTEMP(S) LEFT : # # # #\n");
  printw("\n");
  clrtoeol();
  refresh();
  int address;
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < columns; ++c) {
      address = start_address + r * column_size + c * rows * column_size;
      printw("0x%04X %s  ", address,
             content[c].substr(r * column_size, column_size).c_str());
    }
    printw("\n");
  }
  printw("\n");
  printw("Press 'q' to exit\n");
  for (std::size_t i = 0; i < words.size(); ++i) {
    printw("[%lu, %lu, %lu] `%s`\n", words[i].column, words[i].start, words[i].end,
           words[i].word.c_str());
  }

  // ==========================================================================
  this->move_cursor_to(0, 0, 0);
  this->update();
  return true;
}

void Game::stop() { endwin(); }

void Game::run() {
  mousemask(ALL_MOUSE_EVENTS, NULL);

  for (int ch = getch(); ch != 'q'; ch = getch()) {
    this->update();
    this->move_cursor(ch);
    refresh();
  }
}

void Game::update() {}

void Game::move_cursor(int key) {
  // Start from the current positions.
  int y = position.y;
  int x = position.x;
  int column = position.column;
  // Check if it was a mouse click.
  if (!this->parse_mouse_position(key, x, y, column)) {
    // Check if it was a valid key.
    if (!this->parse_key_position(key, x, y, column)) {
      return;
    }
  }
  // Update the internal position.
  position.x = x;
  position.y = y;
  position.column = column;
  // Move the cursor.
  this->move_cursor_to(x, y, column);
}

bool Game::parse_mouse_position(int key, int &x, int &y, int &column) const {
  if (key == KEY_MOUSE) {
    MEVENT event;
    if (getmouse(&event) == OK) {
      y = event.y - 5;
      x = event.x % (6 + 1 + column_size + 2) - 7;
      column = (event.x - 7) / (6 + 1 + column_size + 2);
      if ((y >= 0) && (x >= 0) && (y < rows) && (x < column_size)) {
        return true;
      }
    }
  }
  return false;
}

bool Game::parse_key_position(int key, int &x, int &y, int &column) const {
  if ((key == KEY_UP) && (y > 0)) {
    y = y - 1;
  } else if ((key == KEY_DOWN) && (y < rows - 1)) {
    y = y + 1;
  } else if (key == KEY_LEFT) {
    if (x > 0) {
      x = x - 1;
    } else if (column > 0) {
      column = column - 1;
      x = column_size;
    }
  } else if (key == KEY_RIGHT) {
    if (x < column_size - 1) {
      x = x + 1;
    } else if (column < columns - 1) {
      column = column + 1;
      x = 0;
    }
  } else {
    return false;
  }
  return true;
}

void Game::move_cursor_to(int x, int y, int column) const {
  if ((y >= 0) && (x >= 0) && (y < rows) && (x < column_size) &&
      (column >= 0) && (column < columns)) {
    wmove(stdscr, 5 + y,
          (6 + 1) * (column + 1) + (2 + column_size) * (column) + x);
  }
}

bool Game::find_unoccupied_space_for_word(Word &word) const {
  bool overlaps;
  for (std::size_t round = 0; round < 20; ++round) {
    // Place the word.
    word.column = random_number(0, columns - 1);
    word.start = random_number(0, rows * column_size - word.length);
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
  word.column = 0;
  word.start = 0;
  word.end = 0;
  return false;
}

} // namespace robsec
