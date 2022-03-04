#pragma once
#include <ncurses.h>

enum class Move : short {
    NONE, W, A, S, D
};

Move move_of(int c) {
    if (c == KEY_UP || c == 'w' || c == 'W') {
        return Move::W;
    }
    if (c == KEY_DOWN || c == 's' || c == 'S') {
        return Move::S;
    }
    if (c == KEY_LEFT || c == 'a' || c == 'A') {
        return Move::A;
    }
    if (c == KEY_RIGHT || c == 'd' || c == 'D') {
        return Move::D;
    }
    return Move::NONE;
}