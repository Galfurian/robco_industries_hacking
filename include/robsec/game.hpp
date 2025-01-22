/// @file game.hpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief Header file for the RobCo Industries hacking game implementation.

#pragma once

#include <string>
#include <vector>
#include <map>

/// @brief Namespace for the RobCo hacking game.
namespace robsec
{

/// @brief Represents a location within the game grid.
class GameLocation {
public:
    std::size_t panel;  ///< Panel index in the game grid.
    std::size_t column; ///< Column index within the panel.
    std::size_t row;    ///< Row index within the panel.

    /// @brief Constructs a GameLocation with the given panel, column, and row.
    GameLocation(std::size_t _panel, std::size_t _column, std::size_t _row)
        : panel(_panel), column(_column), row(_row)
    {
    }
};

/// @brief Represents a screen location in absolute coordinates.
class ScreenLocation {
public:
    std::size_t x; ///< X-coordinate on the screen.
    std::size_t y; ///< Y-coordinate on the screen.

    /// @brief Constructs a ScreenLocation with the given x and y coordinates.
    ScreenLocation(std::size_t _x, std::size_t _y)
        : x(_x), y(_y)
    {
    }
};

/// @brief Represents a word in the game, including its position and metadata.
class Word {
public:
    std::size_t panel;                       ///< Panel where the word starts.
    std::size_t start;                       ///< Start position of the word.
    std::size_t end;                         ///< End position of the word.
    std::string string;                      ///< The word itself.
    std::vector<ScreenLocation> coordinates; ///< Screen coordinates of the word.

    /// @brief Constructs a Word object with the specified parameters.
    Word(std::size_t _panel, std::size_t _start, std::string _string)
        : panel(_panel), start(_start), end(_start + _string.length()), string(_string), coordinates()
    {
        // Initialization of the word's coordinates.
    }

    /// @brief Clears the word's data, resetting most members to default values,
    /// except for the string itself.
    void reset()
    {
        panel = 0;           // Reset the panel index.
        start = 0;           // Reset the start position.
        end   = 0;           // Reset the end position.
        coordinates.clear(); // Clear the vector of screen coordinates.
    }

    /// @brief Equality operator to compare words by their string value.
    bool operator==(const Word &rhs) const
    {
        return this->string == rhs.string;
    }

    /// @brief Checks if the word overlaps with another word.
    bool overlap(const Word &rhs) const
    {
        return (start <= rhs.end) && (rhs.start <= end);
    }

    /// @brief Checks if the word overlaps with any word in the given list.
    inline bool overlap(const std::vector<Word> &words) const
    {
        for (const auto &other : words) {
            if (this->overlap(other)) {
                return true;
            }
        }
        return false;
    }

    /// @brief Determines if the word is currently selected based on position.
    bool is_selected(std::size_t c, std::size_t position) const
    {
        return (panel == c) && (start <= position) && (position < end);
    }
};

/// @brief Represents a group of words with the same length.
struct DictionaryGroup {
    std::size_t length;             ///< Length of the words in the group.
    std::vector<std::string> words; ///< List of words of this length.

    /// @brief Equality operator to compare groups by their length.
    inline bool operator==(const DictionaryGroup &other) const
    {
        return length == other.length;
    }
};

/// @brief Represents the main game logic for the RobCo hacking emulator.
class Game {
private:
    std::string dictionary_path;                    ///< Path to the dictionary file.
    std::vector<std::string> dictionary;            ///< The full list of dictionary words.
    std::vector<DictionaryGroup> sorted_dictionary; ///< Dictionary sorted by word length.
    std::size_t start_address;                      ///< Starting address for the game display.
    std::size_t n_panels;                           ///< Number of panels in the game.
    std::size_t n_rows;                             ///< Number of rows per panel.
    std::size_t n_columns;                          ///< Number of columns per panel.
    std::size_t n_words;                            ///< Number of words in the game.
    int attempts_max;                               ///< Maximum number of allowed attempts.
    int attempts;                                   ///< Remaining number of attempts.
    GameLocation position;                          ///< Current cursor position.
    std::string solution;                           ///< Correct word to guess.
    std::vector<Word> words;                        ///< Words used in the game.
    std::vector<std::string> content;               ///< Panel contents for display.
    enum GameState {
        Running,      ///< Game is running.
        MousePressed, ///< Mouse button pressed.
        EnterPressed, ///< Enter key pressed.
        Won,          ///< Game won.
        Lost,         ///< Game lost.
    } state;          ///< Current game state.

public:
    /// @brief Constructs the Game object with configuration parameters.
    Game(std::string _dictionary_path, std::size_t _n_panels, std::size_t _n_rows, std::size_t _n_columns, std::size_t _n_words, int _attempts_max);

    /// @brief Initializes the game, including loading the dictionary and setting up the grid.
    bool initialize();

    /// @brief Stops the game and resets resources.
    void stop();

    /// @brief Main game loop for handling events and rendering.
    bool run();
    

private:
    /// @brief Renders the game screen.
    bool render();

    /// @brief Handles input from the user.
    void parse_input(int key);

    /// @brief Parses mouse input and updates the location accordingly.
    bool parse_mouse_position(int key, GameLocation &location) const;

    /// @brief Parses keyboard input and updates the location accordingly.
    bool parse_key_position(int key, GameLocation &location) const;

    /// @brief Finds a valid position for a word that doesn't overlap with others.
    bool find_unoccupied_space_for_word(Word &word) const;

    /// @brief Computes the memory address for a given row and panel.
    std::size_t compute_address(std::size_t row, std::size_t panel) const;

    /// @brief Converts a GameLocation to a ScreenLocation.
    ScreenLocation to_screen_location(const GameLocation &location) const;

    /// @brief Converts a ScreenLocation to a GameLocation.
    GameLocation to_game_location(const ScreenLocation &location) const;

    /// @brief Converts a linear position to a GameLocation.
    GameLocation linear_to_game_location(std::size_t panel, std::size_t position) const;

    /// @brief Converts a linear position to a ScreenLocation.
    ScreenLocation linear_to_screen_location(std::size_t panel, std::size_t position) const;

    /// @brief Moves the cursor to the specified panel, column, and row.
    void move_cursor_to(std::size_t panel, std::size_t column, std::size_t row) const;

    /// @brief Moves the cursor to the specified GameLocation.
    void move_cursor_to(const GameLocation &location) const;

    /// @brief Moves the cursor to the specified ScreenLocation.
    void move_cursor_to(const ScreenLocation &location) const;

    /// @brief Moves the cursor to the specified screen coordinates.
    void move_cursor_to(int x, int y) const;

    /// @brief Loads the dictionary from the specified path.
    bool load_dictionary();

    /// @brief Finds the currently selected word, if any.
    const Word *find_selected_word() const;
};

} // namespace robsec
