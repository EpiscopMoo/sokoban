#pragma once
#include "Cell.hpp"
#include "Point.hpp"
#include "Move.hpp"

#include <vector>
#include <string>
#include <stdexcept>
#include <optional>

class Level {
public:
    Level(const Level& other) = delete;
    Level(const Level&& other) = delete;
    Level(const std::vector<std::string>& _strs) : strs(_strs) {}

    std::optional<Cell> next(Point p, Move move) const {
        switch (move) {
            case Move::W:
                return at(p.x - 1, p.y);
            case Move::A:
                return at(p.x, p.y - 1);
            case Move::S:
                return at(p.x + 1, p.y);
            case Move::D:
                return at(p.x, p.y + 1);
            default:
                return std::nullopt;
        }
    }

    std::vector<Cell> adjacent_walkable(Point p) const {
        std::vector<Cell> result;
        result.reserve(MOVES.size());
        for (auto move : MOVES) {
            if (auto o_next = next(p, move); o_next && o_next->type != CellType::WALL) {
                result.push_back(*o_next);
            }
        }
        return result;
    }

    std::optional<Cell> at(Point p) const {
        return at(p.x, p.y);
    }

    const std::vector<std::string>& as_printable_strs() const {
        return strs;
    }

    Point dimensions() const {
        return Point{ strs.size(), strs[0].length() };
    }
private:
    static constexpr auto MOVES = { Move::W, Move::A, Move::S, Move::D };
    std::vector<std::string> strs;

    std::optional<Cell> at(size_t i, size_t j) const {
        if (i >= strs.size() || j >= strs[0].length()) {
            return std::nullopt;
        }
        return std::optional<Cell>(Cell { from_char(strs[i][j]), Point{i, j} });
    }

    static constexpr CellType from_char(char c) {
        if (c == '#') return CellType::WALL;
        if (c == '.') return CellType::TARGET;
        return CellType::NONE;
    }
};


