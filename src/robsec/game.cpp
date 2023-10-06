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

template <typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator &g)
{
    long ub = std::distance(start, end) - 1;
    if (ub > 0) {
        std::uniform_int_distribution<long> dis(0, ub);
        std::advance(start, dis(g));
    }
    return start;
}

template <typename Iter>
Iter select_randomly(Iter start, Iter end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

template <typename T>
static inline std::string &string_modifier(std::string &s, T (*fun)(T))
{
    for (auto &c : s) {
        c = static_cast<char>(fun(c));
    }
    return s;
}

static inline int count_common_letters(const char *a, const char *b)
{
    int table[256] = { 0 };
    int result     = 0;
    for (; *a; a++) table[static_cast<unsigned char>(*a)]++;
    for (; *b; b++) result += (table[static_cast<unsigned char>(*b)]-- > 0);
    return result;
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
    //
    std::srand(static_cast<unsigned>(std::time(NULL)));
    // Compute the starting address.
    start_address = random_number<std::size_t>(0xA000, 0xFFFF - n_rows * n_panels * n_columns);
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
    content.resize(n_panels);
    // Fill the panel content.
    for (std::size_t c = 0; c < n_panels; ++c) {
        content[c] = generate_garbage_string(n_rows * n_columns);
    }
}

bool Game::initialize()
{
    // ==========================================================================
    // Load the dictionary.
    if (!this->load_dictionary()) {
        return false;
    }
    // ==========================================================================
    // Get a random bucket.
    const auto &group = select_randomly(sorted_dictionary.begin(), sorted_dictionary.end());
    // Select and place the words.
    int total_words = static_cast<int>(n_words), round = 10;
    while (total_words && round) {
        // Get a random word.
        std::string string = group->words[random_number<std::size_t>(0, group->words.size() - 1)];
        // Prepare the word object.
        Word word(0, 0, string);
        // Check if we already selected this word.
        if (std::find(words.begin(), words.end(), word) == words.end()) {
            // Find a random place.
            if (this->find_unoccupied_space_for_word(word)) {
                total_words--;
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
    // Choose the solution.
    std::size_t solution_idx = random_number<std::size_t>(0, words.size() - 1);
    solution                 = words[solution_idx].string;
    // Render the scene.
    this->render();
    // Move to cursor at the beginning.
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

void Game::render()
{
    // Find the currently selected word.
    const Word *selected_word = this->find_selected_word();
    int common_letters        = 0;
    // Check if we just pressed enter or the mouse.
    if (selected_word && ((state == MousePressed) || (state == EnterPressed))) {
        if (selected_word->string == solution) {
            state = Won;
            return;
        }
        common_letters = count_common_letters(selected_word->string.c_str(), solution.c_str());
        --attempts;
        if (attempts < 0) {
            state = Lost;
            return;
        }
    }
    // Move the cursor at the beginning.
    wmove(stdscr, 0, 0);
    // Print the header.
    printw(HEADER);
    // Print attempts.
    printw("%d ATTEMP(S) LEFT :", attempts);
    for (int i = 0; i < attempts; ++i) {
        printw(" #");
    }
    printw("\n\n");
    std::size_t address;
    for (std::size_t r = 0; r < n_rows; ++r) {
        for (std::size_t c = 0; c < n_panels; ++c) {
            // Compute the address.
            address = this->compute_address(r, c);
            // Print the address.
            printw("0x%04zX ", address);
            // Print the content.
            printw("%s", content[c].substr(r * n_columns, n_columns).c_str());
            printw("  ");
        }
        printw("\n");
    }
    printw("\n");
    printw("Press 'q' to exit\n");
    for (const auto &word : words) {
        bool is_selected = (selected_word && selected_word->string == word.string);
        if (is_selected) {
            attron(A_REVERSE);
        }
        for (std::size_t j = 0; j < word.string.length(); ++j) {
            mvaddch(static_cast<int>(word.coordinates[j].y), static_cast<int>(word.coordinates[j].x), word.string[j]);
        }
        if (is_selected) {
            attroff(A_REVERSE);
        }
        if (is_selected && ((state == MousePressed) || (state == EnterPressed))) {
            wmove(stdscr, getcury(stdscr) + (attempts_max - attempts - 1) * 2, getcurx(stdscr));
            printw("> %s\n", word.string.c_str());
            printw("> Entry denied, %d correct.\n", common_letters);
        }
    }
    state = Running;
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
            ScreenLocation coord(static_cast<std::size_t>(event.x), static_cast<std::size_t>(event.y));
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
    word.panel = 0;
    word.start = 0;
    word.end   = 0;
    word.coordinates.clear();
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
        return false;
    }
    for (std::size_t i = 0; i < 256; ++i) {
        sorted_dictionary.emplace_back(DictionaryGroup{ i, {} });
    }
    // Clean the dictionary.
    dictionary.clear();
    // Load the dictionary.
    std::string word;
    while (file >> word) {
        // displaying content
        dictionary.emplace_back(string_modifier(word, toupper));
        sorted_dictionary[word.length()].words.push_back(string_modifier(word, toupper));
    }
    for (auto it = sorted_dictionary.cbegin(); it != sorted_dictionary.cend();) {
        if (it->words.size() <= (2 * n_words)) {
            it = sorted_dictionary.erase(it);
        } else {
            ++it;
        }
    }
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
