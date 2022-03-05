#pragma once
#include "Level.hpp"
#include "Move.hpp"
#include <unordered_set>
#include <set>
#include <functional>
#include <ranges>
#include <utility>

struct PushableBox {
    Point crate_pos;
    std::vector<Move> allowed_moves;
};

struct ReducedState {
    ReducedState(const std::unordered_set<Point>&  _boxes) : boxes(_boxes.begin(), _boxes.end()) {}
    bool operator==(const ReducedState& other) const {
        return boxes == other.boxes;
    }
    size_t hash() const {
        size_t r = 0;
        for (Point box : boxes) {
            r = hash_combine(r, box.hash());
        }
        return r;
    }
private:
    std::set<Point> boxes;
};

HASH_SUPPORT(ReducedState)

class GameState {
public:
    GameState(const Level& _level,
              Point _initial_player_position,
              const std::vector<Point>& _box_positions
    ) : level(_level), player_position(_initial_player_position), boxes(_box_positions.begin(), _box_positions.end()) {}
    GameState(const GameState& other) = default;
    GameState& operator=(const GameState& other) {
        player_position = other.player_position;
        boxes = other.boxes;
        return *this;
    }

    void issue_order(Move move) {
        // pick the cell that would've been visited if we performed the move
        if (auto o_cell = level.next(player_position, move); o_cell) {
            Cell cell = *o_cell;

            bool move_allowed = cell.type != CellType::WALL;
            if (is_box(cell)) {
                // if this cell contains a crate we should also check whether it's possible to move the crate as well
                if (auto o_next_for_box = level.next(cell.pos, move); o_next_for_box) {
                    Cell next_for_box = *o_next_for_box;
                    // move is allowed iff the box is not blocked by another box or a wall
                    if (!is_box(next_for_box) && next_for_box.type != CellType::WALL) {
                        move_box(cell.pos, next_for_box.pos);
                    } else { // box cannot be pushed - don't move it and forbid initial player move as well
                        move_allowed = false;
                    }
                } else { // box cannot be pushed out of level boundaries
                    move_allowed = false;
                }
            }

            if (move_allowed) {
                player_position = cell.pos;
            }
        }
    }

    void issue_orders(const std::vector<Move>& orders) {
        for (Move order : orders) {
            issue_order(order);
        }
    }

    std::vector<std::string> as_printable_strs() const {
        std::vector<std::string> level_strs = level.as_printable_strs();
        level_strs[player_position.x][player_position.y] = '@';
        for (auto p : boxes) {
            level_strs[p.x][p.y] = 'x';
        }
        return level_strs;
    }

    bool is_victory() const {
        for (auto p : boxes) {
            auto o_cell = level.at(p);
            if (!o_cell || o_cell->type != CellType::TARGET) {
                return false;
            }
        }
        return true;
    }

    std::function<std::vector<Point>(Point)> f_adjacent_walkable() const {
        return [this] (Point p) -> std::vector<Point> {
            return adjacent_walkable(p);
        };
    }

    std::vector<Point> adjacent_walkable(Point p) const {
        std::vector<Point> result;
        result.reserve(4);
        for (const auto& probably_walkable : level.adjacent_walkable(p)) {
            if (is_walkable(probably_walkable) && probably_walkable.pos != player_position) {
                result.push_back(probably_walkable.pos);
            }
        }
        return result;
    }

    const std::unordered_set<Point>& box_positions() const {
        return boxes;
    }

    std::vector<PushableBox> all_pushable_boxes() const {
        std::vector<PushableBox> result;
        result.reserve(boxes.size());

        std::vector<Move> move_buffer;

        for (auto box : boxes) {
            auto o_up = level.next(box, Move::W);
            auto o_left = level.next(box, Move::A);
            auto o_down = level.next(box, Move::S);
            auto o_right = level.next(box, Move::D);

            bool up = o_up && is_walkable(*o_up);
            bool left = o_left && is_walkable(*o_left);
            bool down = o_down && is_walkable(*o_down);
            bool right = o_right && is_walkable(*o_right);

            // it's very convenient to fail fast here if we have a crate stuck in a corner in a non-target cell
            bool wall_up = !o_up || o_up->type == CellType::WALL;
            bool wall_left = !o_left || o_left->type == CellType::WALL;
            bool wall_down = !o_down || o_down->type == CellType::WALL;
            bool wall_right = !o_right || o_right->type == CellType::WALL;

            if (auto box_cell_type = level.at(box)->type; box_cell_type != CellType::TARGET) {
                if (wall_up && wall_right || wall_right && wall_down || wall_down && wall_left || wall_left && wall_up) {
                    return {}; // forbid all moves
                }
            }

            // otherwise continue gathering available moves around crates
            move_buffer.clear();
            if (up && down) {
                move_buffer.push_back(Move::W);
                move_buffer.push_back(Move::S);
            }
            if (left && right) {
                move_buffer.push_back(Move::A);
                move_buffer.push_back(Move::D);
            }
            if (!move_buffer.empty()) {
                result.push_back(PushableBox{box, move_buffer});
            }
        }

        return result;
    }

    Point player_pos() const {
        return player_position;
    }

    size_t hash() const {
        std::vector<size_t> hashes;
        hashes.reserve(boxes.size());

        for (Point box : boxes) {
            hashes.push_back(std::hash<Point>()(box));
        }

        std::sort(hashes.begin(), hashes.end());

        size_t r = hash_combine(0, std::hash<Point>()(player_position));
        for (size_t h : hashes) {
            r = hash_combine(r, h);
        }
        return r;
    }

    bool operator==(const GameState& other) const {
        return player_position == other.player_position && boxes == other.boxes;
    }

    ReducedState reduced_state() const {
        return ReducedState(boxes);
    }

    size_t count_boxes_on_target() const {
        size_t count = 0;
        for (auto box : boxes) {
            if (auto o_cell = level.at(box); o_cell && o_cell->type == CellType::TARGET) {
                ++count;
            }
        }
        return count;
    }
private:
    const Level& level;
    Point player_position;
    std::unordered_set<Point> boxes;
    bool is_box(Cell p) const {
        return boxes.contains(p.pos);
    }
    bool is_walkable(Cell c) const {
        return c.type != CellType::WALL && !is_box(c);
    }
    void move_box(Point from, Point to) {
        boxes.erase(from);
        boxes.insert(to);
    }
};

HASH_SUPPORT(GameState)