/// @file game.hpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#pragma once

#include <string>
#include <vector>

namespace robsec {

class Word {
public:
  int column;
  int start;
  int end;
  std::string word;

  Word(int _column, int _start, std::string _word)
      : column(_column), start(_start), end(_start + _word.length()),
        word(_word) {
    // Nothing to do.
  }
  bool operator==(const Word &rhs) const { return this->word == rhs.word; }
  bool overlap(const Word &rhs) const { return this->word == rhs.word; }
};

class Game {
private:
  std::string dictionary_path;
  int start_address;
  int columns;
  int rows;
  int column_size;
  struct {
    int x;
    int y;
    int column;
  } position;
  std::vector<Word> words;
  std::vector<std::string> content;
  std::vector<std::string> dictionary;

public:
  Game(std::string _dictionary_path, int _columns, int _rows, int _column_size);

  bool initialize();

  void stop();

  void run();

private:
  void update();

  void move_cursor(int key);

  bool parse_mouse_position(int key, int &x, int &y, int &column) const;

  bool parse_key_position(int key, int &x, int &y, int &column) const;

  void move_cursor_to(int x, int y, int column) const;

  bool check_if_occupied_by_word(int column, int position, int length) const;
};

} // namespace robsec
