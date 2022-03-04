#pragma once
#include "GameState.hpp"

#include <ncurses.h>
#include <unistd.h>

class InterLayer {
public:
    InterLayer(GameState& _game) : game(_game) {}
    void init_screen() {
        initscr();
        noecho();
        cbreak();
        keypad(stdscr, TRUE);
        curs_set(0);
    }
    void manual_loop() {
        int ch;
        while ((ch = getch()) != 'x') {
            game.issue_order(move_of(ch));
            clear();
            draw();
            if (game.is_victory()) {
                printw("Victory!");
            }
            printw("\nPress 'x' to exit");
            refresh();
        }
    }
    void execute_commands(const std::vector<Move>& moves) {
        for (auto move : moves) {
            usleep(500000);
            game.issue_order(move);
            clear();
            draw();
            if (game.is_victory()) {
                printw("Victory!");
            }
            printw("\nAutomatic mode");
            refresh();
        }
        int ch;
        while ((ch = getch()) != 'x') {
            clear();
            draw();
            printw("\nDone. Press 'x' to exit");
            refresh();
        }
    }
    ~InterLayer() {
        endwin();
    }
private:
    GameState& game;
    void draw() {
        for (const auto& line : game.as_printable_strs()) {
            std::string widened;
            for (auto c : line) {
                widened += c;
                widened += ' ';
            }
            printw(widened.c_str());
            addch('\n');
        }
    }
};