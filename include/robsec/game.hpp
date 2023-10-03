/// @file game.hpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#pragma once

#include <string>
#include <vector>

namespace robsec
{

class GameLocation {
public:
    std::size_t panel;
    std::size_t column;
    std::size_t row;

    GameLocation(std::size_t _panel, std::size_t _column, std::size_t _row)
        : panel(_panel), column(_column), row(_row)
    {
    }
};

class ScreenLocation {
public:
    std::size_t x;
    std::size_t y;

    ScreenLocation(std::size_t _x, std::size_t _y)
        : x(_x), y(_y)
    {
    }
};

class Word {
public:
    std::size_t panel;
    std::size_t start;
    std::size_t end;
    std::string string;
    std::size_t length;
    std::vector<ScreenLocation> coordinates;

    Word(std::size_t _panel, std::size_t _start, std::string _string)
        : panel(_panel), start(_start), end(_start + _string.length()), string(_string),
          length(_string.length()), coordinates()
    {
        // Nothing to do.
    }
    bool operator==(const Word &rhs) const
    {
        return this->string == rhs.string;
    }
    bool overlap(const Word &rhs) const
    {
        return (start <= rhs.end) && (rhs.start <= end);
    }
    bool is_selected(std::size_t c, std::size_t position) const
    {
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
    std::size_t attempts_max;
    std::size_t attempts;
    GameLocation position;
    std::vector<Word> words;
    std::vector<std::string> content;
    std::vector<std::string> dictionary;

public:
    Game(std::string _dictionary_path, std::size_t _panels, std::size_t _rows, std::size_t _panel_size);

    bool initialize();

    void stop();

    void run();

private:
    void update();

    void parse_input(int key);

    bool parse_mouse_position(int key, GameLocation &location) const;

    bool parse_key_position(int key, GameLocation &location) const;

    bool find_unoccupied_space_for_word(Word &word) const;

    std::size_t compute_address(std::size_t row, std::size_t panel) const;

    ScreenLocation to_screen_location(const GameLocation &location) const;

    GameLocation to_game_location(const ScreenLocation &location) const;

    GameLocation linear_to_game_location(std::size_t panel, std::size_t position) const;

    ScreenLocation linear_to_screen_location(std::size_t panel, std::size_t position) const;

    void move_cursor_to(std::size_t panel, std::size_t column, std::size_t row) const;

    void move_cursor_to(const GameLocation &location) const;

    void move_cursor_to(const ScreenLocation &location) const;

    void move_cursor_to(int x, int y) const;
};

} // namespace robsec
