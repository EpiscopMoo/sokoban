#pragma once

#include "Point.hpp"

enum class CellType : short {
    NONE,
    WALL,
    TARGET
};

struct Cell {
    CellType type;
    Point pos;
};