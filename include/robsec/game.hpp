/// @file game.hpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#pragma once

#include <string>
#include <vector>
#include <map>

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
    std::vector<ScreenLocation> coordinates;

    Word(std::size_t _panel, std::size_t _start, std::string _string)
        : panel(_panel), start(_start), end(_start + _string.length()), string(_string), coordinates()
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
    inline bool overlap(const std::vector<Word> &words) const
    {
        for (const auto &other : words) {
            if (this->overlap(other)) {
                return true;
            }
        }
        return false;
    }

    bool is_selected(std::size_t c, std::size_t position) const
    {
        return (panel == c) && (start <= position) && (position < end);
    }
};

struct DictionaryGroup {
    std::size_t length;
    std::vector<std::string> words;

    inline bool operator==(const DictionaryGroup &other) const
    {
        return length == other.length;
    }
};

class Game {
private:
    /// Path to the dictionary.
    std::string dictionary_path;
    /// The full dictionary.
    std::vector<std::string> dictionary;
    /// The full dictionary.
    std::vector<DictionaryGroup> sorted_dictionary;
    /// Starting address when displaying the game.
    std::size_t start_address;
    /// The total number of panels.
    std::size_t n_panels;
    /// The number of rows per panel.
    std::size_t n_rows;
    /// The number of columns per panel.
    std::size_t n_columns;
    /// The number of words in the game.
    std::size_t n_words;
    /// The maximum number of attempts.
    int attempts_max;
    /// The current number of remaining attempts.
    int attempts;
    /// The cursor position.
    GameLocation position;
    /// The correct word inside the words vector.
    std::string solution;
    /// The list of words in the game.
    std::vector<Word> words;
    /// The content of the panels.
    std::vector<std::string> content;
    /// The current game state.
    enum GameState {
        Running,
        MousePressed,
        EnterPressed,
        Won,
        Lost
    } state;

public:
    Game(std::string _dictionary_path, std::size_t _n_panels, std::size_t _n_rows, std::size_t _n_columns, std::size_t _n_words, int _attempts_max);

    bool initialize();

    void stop();

    bool run();

private:
    void render();

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

    bool load_dictionary();

    const Word *find_selected_word() const;
};

} // namespace robsec
