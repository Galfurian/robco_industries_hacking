/// @file game.hpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#pragma once

#include <string>
#include <vector>

namespace robsec {

class screen_coord;

class game_coord {
public:
  std::size_t panel;
  std::size_t row;
  std::size_t column;

  game_coord(std::size_t _panel, std::size_t _row, std::size_t _column)
      : panel(_panel), row(_row), column(_column) {}

  screen_coord to_screen_coord(std::size_t panel_size) const;
};

class screen_coord {
public:
  std::size_t x;
  std::size_t y;

  screen_coord(std::size_t _x, std::size_t _y) : x(_x), y(_y) {}

  game_coord to_game_coord(std::size_t panel_size) const;
};

class Word {
public:
  std::size_t panel;
  std::size_t start;
  std::size_t end;
  std::string word;
  std::size_t length;

  Word(std::size_t _panel, std::size_t _start, std::string _word)
      : panel(_panel), start(_start), end(_start + _word.length()), word(_word),
        length(_word.length()) {
    // Nothing to do.
  }
  bool operator==(const Word &rhs) const { return this->word == rhs.word; }
  bool overlap(const Word &rhs) const {
    return (start <= rhs.end) && (rhs.start <= end);
  }
  bool is_selected(std::size_t c, std::size_t position) const {
    return (panel == c) && (start <= position) && (position < end);
  }
};

class Game {
private:
  std::string dictionary_path;
  std::size_t start_address;
  std::size_t panels;
  std::size_t rows;
  std::size_t panel_size;
  game_coord position;
  std::vector<Word> words;
  std::vector<std::string> content;
  std::vector<std::string> dictionary;

public:
  Game(std::string _dictionary_path, std::size_t _panels, std::size_t _rows,
       std::size_t _panel_size);

  bool initialize();

  void stop();

  void run();

private:
  void update();

  void print_char(std::size_t &panel, std::size_t &x, std::size_t &y, char c);

  void parse_input(int key);

  bool parse_mouse_position(int key, std::size_t &x, std::size_t &y,
                            std::size_t &panel) const;

  bool parse_key_position(int key, std::size_t &x, std::size_t &y,
                          std::size_t &panel) const;

  void move_cursor_to(std::size_t x, std::size_t y, std::size_t panel) const;

  bool find_unoccupied_space_for_word(Word &word) const;

  std::size_t compute_address(std::size_t row, std::size_t panel) const;

  inline void game_coord_to_screen_coord(std::size_t &panel, std::size_t &r,
                                         std::size_t &c, std::size_t &x,
                                         std::size_t &y,
                                         std::size_t &panel_size) const {
    y = 5 + r;
    x = (6 + 1) * (panel + 1) + (2 + panel_size) * (panel) + c;
  }

  inline void screen_coord_to_game_coord(std::size_t &panel, std::size_t &r,
                                         std::size_t &c, std::size_t &x,
                                         std::size_t &y) const {
    y = 5 + r;
    x = (6 + 1) * (panel + 1) + (2 + panel_size) * (panel) + c;
  }
};

} // namespace robsec
