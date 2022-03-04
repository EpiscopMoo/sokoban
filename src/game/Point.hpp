#pragma once
#include "../util/Hash.hpp"
#include "Move.hpp"
#include <cstddef>

struct Point {
    size_t x, y;
    bool operator==(const Point& other) const {
        return &other == this || (other.x == x && other.y == y);
    }
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
    static size_t distance(Point p1, Point p2) {
        return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
    }
    size_t hash() const {
        return hash_combine(hash_combine(0, x), x);
    }
    Point move(Move m) const {
        switch (m) {
            case Move::W:
                return {x - 1, y};
            case Move::A:
                return {x, y - 1};
            case Move::S:
                return {x + 1, y};
            case Move::D:
                return {x, y + 1};
            default:
                return *this;
        }
    }
};

HASH_SUPPORT(Point)