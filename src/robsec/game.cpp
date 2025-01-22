/// @file game.cpp
/// @author Enrico Fraccaroli (enry.frak@gmail.com)
/// @brief

#include "robsec/game.hpp"

#include <cstdint>
#include <curses.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

/// @brief Length of an address in the game.
#define ADDRESS_LEN 6

/// @brief Header displayed at the start of the game.
#define HEADER                                  \
    "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL\n" \
    "ENTER PASSWORD NOW\n"                      \
    "\n"

/// @brief Length of the header in terms of newlines and additional spacing.
#define HEADER_LEN (3 + 2)

/// @brief Macro to check the result of an expression and return false if it fails.
#define CHECK_AND_REPORT(expr, msg)                     \
    do {                                                \
        if ((expr) == ERR) {                            \
            std::cerr << "Error: " << msg << std::endl; \
            return false;                               \
        }                                               \
    } while (0)

/// @brief Generates a random number within the specified range.
///
/// @tparam T The type of the range values (e.g., int, float).
/// @param min The minimum value of the range (inclusive).
/// @param max The maximum value of the range (inclusive).
/// @return A random number of type T within the specified range.
template <typename T>
static inline T random_number(T min, T max)
{
    // Random device used to seed the generator.
    std::random_device seeder;
    // Mersenne Twister engine for generating random numbers.
    std::mt19937 engine(seeder());
    // Uniform distribution for numbers within the given range.
    std::uniform_int_distribution<T> dist(min, max);
    return dist(engine); // Generate and return the random number.
}

/// @brief Generates a random string of garbage characters.
///
/// @param width The length of the string to generate.
/// @return A string containing random garbage characters.
static inline std::string generate_garbage_string(std::size_t width)
{
    // Set of characters to use for generating the garbage string.
    static const char *garbage = ",|\\!@#$%^&*-_+=.:;?,/";
    std::string s;

    // Generate a random character for each position in the string.
    for (std::size_t i = 0; i < width; ++i) {
        // Select a random character from the garbage array.
        s.push_back(garbage[random_number<std::size_t>(0, std::strlen(garbage) - 1)]);
    }

    return s; // Return the generated garbage string.
}

/// @brief Selects a random iterator within a specified range, using a custom random generator.
///
/// @tparam Iter Type of the iterator.
/// @tparam RandomGenerator Type of the random generator.
/// @param start Iterator to the beginning of the range.
/// @param end Iterator to the end of the range.
/// @param g Reference to a random generator.
/// @return A randomly selected iterator within the range.
template <typename Iter, typename RandomGenerator>
static inline Iter select_randomly(Iter start, Iter end, RandomGenerator &g)
{
    using type_t = typename Iter::difference_type;
    type_t ub    = std::distance(start, end) - 1;
    if (ub > 0) {
        // Create a uniform distribution within the range.
        std::uniform_int_distribution<type_t> dis(0, ub);
        // Advance the start iterator by a random offset.
        std::advance(start, dis(g));
    }
    return start; // Return the randomly selected iterator.
}

/// @brief Selects a random iterator within a specified range, using a static random generator.
///
/// @tparam Iter Type of the iterator.
/// @param start Iterator to the beginning of the range.
/// @param end Iterator to the end of the range.
/// @return A randomly selected iterator within the range.
template <typename Iter>
static inline Iter select_randomly(Iter start, Iter end)
{
    // Static random device and generator to avoid re-initialization.
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen); // Use the overloaded version.
}

/// @brief Modifies a string by applying a transformation function to each character.
///
/// @tparam T Type of the transformation function.
/// @param s Reference to the string to be modified.
/// @param fun Function pointer to the transformation function.
/// @return Reference to the modified string.
template <typename T>
static inline std::string &string_modifier(std::string &s, T (*fun)(T))
{
    // Apply the transformation function to each character in the string.
    for (auto &c : s) {
        c = static_cast<char>(fun(c));
    }
    return s; // Return the modified string.
}

/// @brief Counts the number of common letters between two C-style strings.
///
/// @param a Pointer to the first string.
/// @param b Pointer to the second string.
/// @return The count of common letters between the two strings.
static inline int count_common_letters(const char *a, const char *b)
{
    int table[256] = { 0 }; // Frequency table for characters in 'a'.
    int result     = 0;     // Counter for common characters.

    // Populate the frequency table with characters from 'a'.
    for (; *a; a++) {
        table[static_cast<unsigned char>(*a)]++;
    }

    // Count matching characters from 'b' in the frequency table.
    for (; *b; b++) {
        result += (table[static_cast<unsigned char>(*b)]-- > 0);
    }

    return result; // Return the count of common letters.
}

namespace robsec
{

Game::Game(std::string _dictionary_path,
           std::size_t _n_panels,
           std::size_t _n_rows,
           std::size_t _n_columns,
           std::size_t _n_words,
           int _attempts_max)
    : dictionary_path(_dictionary_path),
      dictionary(),
      start_address(0),
      n_panels(_n_panels),
      n_rows(_n_rows),
      n_columns(_n_columns),
      n_words(_n_words),
      attempts_max(_attempts_max),
      attempts(attempts_max),
      position({ 0, 0, 0 }),
      solution(),
      words(),
      content(),
      state(Running)
{
    // Nothing to do.
}

bool Game::initialize()
{
    // Load the dictionary.
    if (!this->load_dictionary()) {
        std::cerr << "Error: Failed to load the dictionary." << std::endl;
        return false;
    }

    // Ensure the sorted dictionary is not empty.
    if (sorted_dictionary.empty()) {
        std::cerr << "Error: The sorted dictionary is empty after loading." << std::endl;
        return false;
    }

    // Get a random dictionary group.
    const DictionaryGroup *dictionary = nullptr;
    try {
        dictionary = &(*select_randomly(sorted_dictionary.begin(), sorted_dictionary.end()));
    } catch (const std::exception &e) {
        std::cerr << "Error: Failed to select a random dictionary group. Exception: " << e.what() << std::endl;
        return false;
    }

    // Ensure the dictionary group contains words.
    if (dictionary->words.empty()) {
        std::cerr << "Error: The selected dictionary group contains no words." << std::endl;
        return false;
    }

    // Create a mutable copy of the dictionary group's words for selection.
    std::vector<std::string> selection = dictionary->words;

    // Ensure we don't attempt to place more words than are available.
    int total_words = std::min(static_cast<int>(n_words), static_cast<int>(selection.size()));

    // Ensure there are words to place.
    if (total_words == 0) {
        std::cerr << "Error: No words available to place after adjustments." << std::endl;
        return false;
    }

    // Keep track of the round.
    int round = 100;

    // Place the words.
    while (total_words && round) {
        // Get a random index and word from the selection.
        std::size_t index  = random_number<std::size_t>(0, selection.size() - 1);
        std::string string = selection[index];

        // Prepare the word object.
        Word word(0, 0, string);

        // Check if we already selected this word.
        if (std::find(words.begin(), words.end(), word) == words.end()) {
            // Find a random place.
            if (this->find_unoccupied_space_for_word(word)) {
                total_words--;                              // Decrease the count of words to place.
                words.emplace_back(word);                   // Add the word to the placed words list.
                selection.erase(selection.begin() + index); // Remove the word from the selection.
                continue;                                   // Skip the rest of the loop for this iteration.
            }
        }
        // Decrease the round count to prevent infinite loops.
        round--;
    }

    // Check if the placement process failed.
    if (round == 0) {
        std::cerr << "Error: Failed to place all words within the allowed rounds." << std::endl;
        return false;
    }

    // Ensure there are words placed in the game.
    if (words.empty()) {
        std::cerr << "Error: No words were placed in the game." << std::endl;
        return false;
    }

    // Choose the solution.
    std::size_t solution_idx = random_number<std::size_t>(0, words.size() - 1);
    solution                 = words[solution_idx].string;

    // Seed the random number generator.
    std::srand(static_cast<unsigned>(std::time(NULL)));

    // Compute the starting address.
    start_address = random_number<std::size_t>(0xA000, 0xFFFF - n_rows * n_panels * n_columns);

    // Initialize NCurses.
    if (initscr() == nullptr) {
        std::cerr << "Error: Failed to initialize NCurses." << std::endl;
        return false;
    }

    // Clear the screen.
    if (clear() == ERR) {
        std::cerr << "Error: Failed to clear the screen in NCurses." << std::endl;
        endwin();
        return false;
    }

    // Disable echo.
    if (noecho() == ERR) {
        std::cerr << "Error: Failed to disable echo in NCurses." << std::endl;
        endwin();
        return false;
    }

    // Enable cbreak mode.
    if (cbreak() == ERR) {
        std::cerr << "Error: Failed to enable cbreak mode in NCurses." << std::endl;
        endwin();
        return false;
    }

    // Enable keypad input for the main window.
    if (keypad(stdscr, true) == ERR) {
        std::cerr << "Error: Failed to enable keypad input in NCurses." << std::endl;
        endwin();
        return false;
    }

    // Initialize colors in NCurses.
    if (start_color() == ERR) {
        std::cerr << "Error: Failed to initialize colors in NCurses." << std::endl;
        endwin();
        return false;
    }

    // Enable support for default background color.
    if (use_default_colors() == ERR) {
        std::cerr << "Error: Failed to enable default terminal background color." << std::endl;
        endwin();
        return false;
    }

    // Define a color pair for yellow text with a transparent (default) background.
    if (init_pair(1, COLOR_YELLOW, -1) == ERR) {
        std::cerr << "Error: Failed to define color pair 1 (yellow on default background)." << std::endl;
        endwin();
        return false;
    }

    // Initialize the panel content.
    try {
        content.resize(n_panels);
    } catch (const std::exception &e) {
        std::cerr << "Error: Failed to resize content vector. Exception: " << e.what() << std::endl;
        endwin();
        return false;
    }

    // Fill the panel content with garbage strings.
    try {
        for (std::size_t c = 0; c < n_panels; ++c) {
            content[c] = generate_garbage_string(n_rows * n_columns);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: Failed to generate garbage strings for panel content. Exception: " << e.what() << std::endl;
        endwin();
        return false;
    }

    // Render the scene.
    if (!this->render()) {
        std::cerr << "Error: Failed to render the game scene." << std::endl;
        endwin();
        return false;
    }

    // Move the cursor to the beginning.
    this->move_cursor_to(0, 0, 0);

    return true;
}

void Game::stop()
{
    endwin();
}

bool Game::run()
{
    mousemask(ALL_MOUSE_EVENTS, NULL);
    for (int ch = getch(); ch != 'q'; ch = getch()) {
        // Parse the input.
        this->parse_input(ch);
        // Render the scene.
        this->render();
        // Move the cursor.
        this->move_cursor_to(position);
        refresh();
        if (state == Won) {
            return true;
        }
        if (state == Lost) {
            return false;
        }
    }
    return false;
}

bool Game::render()
{
    // Find the currently selected word.
    const Word *selected_word = this->find_selected_word();
    if (!selected_word && ((state == MousePressed) || (state == EnterPressed))) {
        std::cerr << "Error: No word is selected, but input was detected." << std::endl;
        return false;
    }

    int common_letters = 0;

    // Check if we just pressed Enter or the mouse.
    if (selected_word && ((state == MousePressed) || (state == EnterPressed))) {
        if (selected_word->string == solution) {
            state = Won;
            return true; // Exit early if the correct solution is found.
        }

        // Count common letters.
        common_letters = count_common_letters(selected_word->string.c_str(), solution.c_str());

        // Decrease attempts and check if the game is lost.
        if (--attempts == 0) {
            state = Lost;
            return true; // Game ends when attempts run out.
        }
    }

    // Check if the cursor movement fails.
    if (wmove(stdscr, 0, 0) == ERR) {
        std::cerr << "Error: Failed to move the cursor to the beginning." << std::endl;
        return false;
    }

    // Print the header.
    if (printw(HEADER) == ERR) {
        std::cerr << "Error: Failed to print the game header." << std::endl;
        return false;
    }

    // Print attempts.
    if (printw("%d ATTEMPT(S) LEFT :", attempts) == ERR) {
        std::cerr << "Error: Failed to print the remaining attempts." << std::endl;
        return false;
    }
    for (int i = 0; i < attempts; ++i) {
        if (printw(" #") == ERR) {
            std::cerr << "Error: Failed to print the attempt marker." << std::endl;
            return false;
        }
    }
    if (printw("\n\n") == ERR) {
        std::cerr << "Error: Failed to print the attempts separator." << std::endl;
        return false;
    }

    std::size_t address;
    for (std::size_t r = 0; r < n_rows; ++r) {
        for (std::size_t c = 0; c < n_panels; ++c) {
            // Compute the address.
            address = this->compute_address(r, c);

            // Print the address.
            if (printw("0x%04zX ", address) == ERR) {
                std::cerr << "Error: Failed to print the address for row " << r << ", panel " << c << "." << std::endl;
                return false;
            }

            // Print the content.
            if (printw("%s", content[c].substr(r * n_columns, n_columns).c_str()) == ERR) {
                std::cerr << "Error: Failed to print the panel content for row " << r << ", panel " << c << "." << std::endl;
                return false;
            }

            if (printw("  ") == ERR) {
                std::cerr << "Error: Failed to print spacing for row " << r << ", panel " << c << "." << std::endl;
                return false;
            }
        }
        if (printw("\n") == ERR) {
            std::cerr << "Error: Failed to print row separator." << std::endl;
            return false;
        }
    }
    if (printw("\nPress 'q' to exit\n") == ERR) {
        std::cerr << "Error: Failed to print exit prompt." << std::endl;
        return false;
    }

    int x_offset = getcurx(stdscr), y_offset = getcury(stdscr);
    for (const auto &word : words) {
        // Check if the word is selected.
        bool is_selected = (selected_word && selected_word->string == word.string);

        // Enable reverse video for selected words.
        if (is_selected) {
            if (attron(A_REVERSE) == ERR) {
                std::cerr << "Error: Failed to enable reverse video attribute." << std::endl;
                return false;
            }
        }
        // Enable yellow color for unselected words.
        else {
            if (attron(COLOR_PAIR(1)) == ERR) {
                std::cerr << "Error: Failed to enable yellow color for unselected word." << std::endl;
                return false;
            }
        }

        // Print each character of the word.
        for (std::size_t j = 0; j < word.string.length(); ++j) {
            if (mvaddch(static_cast<int>(word.coordinates[j].y), static_cast<int>(word.coordinates[j].x), word.string[j]) == ERR) {
                std::cerr << "Error: Failed to add character for word '" << word.string << "' at position " << j << "." << std::endl;
                return false;
            }
        }

        // Disable the color or reverse video attributes.
        if (is_selected) {
            if (attroff(A_REVERSE) == ERR) {
                std::cerr << "Error: Failed to disable reverse video attribute." << std::endl;
                return false;
            }
        } else {
            if (attroff(COLOR_PAIR(1)) == ERR) {
                std::cerr << "Error: Failed to disable yellow color for unselected word." << std::endl;
                return false;
            }
        }

        if (is_selected && ((state == MousePressed) || (state == EnterPressed))) {
            if (wmove(stdscr, y_offset + (attempts_max - attempts - 1) * 2, x_offset) == ERR) {
                std::cerr << "Error: Failed to move the cursor for word feedback." << std::endl;
                return false;
            }

            if (printw("> %s\n", word.string.c_str()) == ERR) {
                std::cerr << "Error: Failed to print the selected word feedback." << std::endl;
                return false;
            }

            if (printw("> Entry denied, %d correct.\n", common_letters) == ERR) {
                std::cerr << "Error: Failed to print the feedback for common letters." << std::endl;
                return false;
            }
        }
    }

    state = Running; // Set the game state to running.
    return true;     // Indicate success.
}

void Game::parse_input(int key)
{
    // Check if it was a mouse click.
    if (this->parse_mouse_position(key, position)) {
        state = MousePressed;
    } else if (!this->parse_key_position(key, position)) {
        if (key == 10) {
            state = EnterPressed;
        }
    }
}

bool Game::parse_mouse_position(int key, GameLocation &location) const
{
    if (key == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK) {
            // Transform the coordinates into a screen location.
            ScreenLocation coord{ static_cast<std::size_t>(event.x), static_cast<std::size_t>(event.y) };
            // Transform the screen location to a game location.
            GameLocation new_location = this->to_game_location(coord);
            // If the new location is a valid one, save it.
            if ((new_location.panel < n_panels) && (new_location.row < n_rows) && (new_location.column < n_columns)) {
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
    } else if ((key == KEY_DOWN) && (location.row < n_rows - 1)) {
        ++location.row;
    } else if (key == KEY_LEFT) {
        if (location.column > 0) {
            --location.column;
        } else if (location.panel > 0) {
            --location.column = n_columns - 1;
            --location.panel;
        }
    } else if (key == KEY_RIGHT) {
        if (location.column < n_columns - 1) {
            ++location.column;
        } else if (location.panel < n_panels - 1) {
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
    for (std::size_t round = 0; round < 20; ++round) {
        // Place the word.
        word.panel = random_number<std::size_t>(0, n_panels - 1);
        word.start = random_number<std::size_t>(0, n_rows * n_columns - word.string.length());
        word.end   = word.start + word.string.length();
        // Check if it overlaps with another word.
        if (!word.overlap(words)) {
            // Compute the coordinates from the linear location of the word,
            // this will save us some time when we need to highlight a word.
            for (std::size_t i = 0; i < word.string.length(); ++i) {
                word.coordinates.emplace_back(this->linear_to_screen_location(word.panel, word.start + i));
            }
            return true;
        }
    }
    // Reset the word data.
    word.reset();
    return false;
}

std::size_t Game::compute_address(std::size_t row, std::size_t panel) const
{
    return start_address + row * n_columns + panel * n_rows * n_columns;
}

ScreenLocation Game::to_screen_location(const GameLocation &location) const
{
    return ScreenLocation((ADDRESS_LEN + 1) * (location.panel + 1) + (2 + n_columns) * location.panel + location.column,
                          HEADER_LEN + location.row);
}

GameLocation Game::to_game_location(const ScreenLocation &location) const
{
    return GameLocation((location.x - ADDRESS_LEN - 1) / (ADDRESS_LEN + 1 + n_columns + 2),
                        location.x % (ADDRESS_LEN + 1 + n_columns + 2) - ADDRESS_LEN - 1, location.y - HEADER_LEN);
}

GameLocation Game::linear_to_game_location(std::size_t panel, std::size_t position) const
{
    return GameLocation(panel, position % n_columns, position / n_columns);
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

bool Game::load_dictionary()
{
    // Open the dictionary file.
    std::ifstream file(dictionary_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open dictionary file: " << dictionary_path << std::endl;
        return false; // Return false if the file cannot be opened.
    }

    // Initialize the sorted dictionary with 256 groups (one for each possible length).
    sorted_dictionary.clear();
    sorted_dictionary.reserve(256);
    for (std::size_t i = 0; i < 256; ++i) {
        sorted_dictionary.emplace_back(DictionaryGroup{ i, {} });
    }

    // Clean the main dictionary.
    dictionary.clear();

    // Load the dictionary.
    std::string word;

    while (file >> word) {
        // Validate that the word is not empty before processing.
        if (word.empty()) {
            std::cerr << "Encountered an empty word in the dictionary. Skipping..." << std::endl;
            continue;
        }

        // Modify the word to uppercase.
        std::string modified_word = string_modifier(word, toupper);

        // Ensure the word fits within the expected length bounds.
        if (modified_word.length() >= sorted_dictionary.size()) {
            std::cerr << "Word '" << modified_word << "' exceeds the maximum supported length. Skipping..." << std::endl;
            continue;
        }

        // Add the word to the main dictionary.
        dictionary.emplace_back(modified_word);

        // Add the word to the appropriate group in the sorted dictionary.
        sorted_dictionary[modified_word.length()].words.push_back(modified_word);
    }

    // Save the total number of groups before the removal process.
    std::size_t total_groups = sorted_dictionary.size();

    // Count how many groups are removed during the process.
    std::size_t removed_groups = 0;

    // Remove dictionary groups with insufficient words.
    for (auto it = sorted_dictionary.begin(); it != sorted_dictionary.end();) {
        if (it->words.size() <= (2 * n_words)) {
            it = sorted_dictionary.erase(it); // Erase groups with too few words.
            removed_groups++;                 // Increment the count of removed groups.
        } else {
            ++it; // Move to the next group.
        }
    }

    // Check if all groups were removed.
    if (removed_groups == total_groups) {
        std::cerr << "Error: All dictionary groups were removed due to n_words being too high (" << n_words << ").\n"
                  << "Total groups before removal: " << total_groups << ", groups removed: " << removed_groups << "." << std::endl;
        return false;
    }

    // Close the file.
    file.close();

    return true;
}

const Word *Game::find_selected_word() const
{
    for (const auto &word : words) {
        if (word.is_selected(position.panel, position.row * n_columns + position.column)) {
            return &word;
        }
    }
    return nullptr;
}

} // namespace robsec
