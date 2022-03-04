#pragma once
#include "../game/Level.hpp"
#include "Paths.hpp"

#include <unordered_map>
#include <utility>

class Solver {
    using NonIsomorphicStates = std::unordered_map<ReducedState, std::vector<GameState>>;
public:
    Solver(const Level& _level) : level(_level) {}
    std::vector<Move> solve(const GameState& state) {
        NonIsomorphicStates states;
        return solve(state, {}, states);
    }
private:
    const Level& level;

    struct NextState {
        GameState state;
        std::vector<Move> moves;
        NextState(const NextState& other) = default;
        NextState(const GameState&  _state, std::vector<Move>  _moves) : state(_state), moves(std::move(_moves)) {}
        NextState& operator=(const NextState& other) {
            state = GameState(other.state);
            moves = other.moves;
            return *this;
        }
        bool operator<(const NextState& other) const {
            return state.count_boxes_on_target() > other.state.count_boxes_on_target();
        }
    };

    std::vector<Move> solve(const GameState& state,
                            const std::vector<Move>& initial_moves,
                            NonIsomorphicStates& states) {
        if (state.is_victory()) {
            return initial_moves;
        }
        if (!validate_state_uniqueness(state, states)) {
            return {};
        }

        // fail fast heuristics
        if (is_unsolvable(state)) {
            return {};
        }

        // we don't really care about empty cells non-adjacent to crates,
        // assuming we can walk straight through them with A*.
        auto pushable_boxes = state.all_pushable_boxes();
        if (pushable_boxes.empty()) {
            return {}; // no solution
        }

        std::vector<Path> reachable_push_positions;
        std::vector<Move> push_commands;
        reachable_push_positions.reserve(pushable_boxes.size());
        push_commands.reserve(pushable_boxes.size());

        // For each crate analyze from which side it can be pushed. Pushing is possible if two conditions are satisfied:
        // 1. push-position provided by game state with respect to walls and other crates
        // 2. position before the crate is reachable by player (or player already stands there)
        prioritise_untargeted_boxes(pushable_boxes);
        for (const PushableBox& pushable_box : pushable_boxes) {
            const auto& allowed_moves = pushable_box.allowed_moves; // (1) valid push positions from game state

            for (Move move : allowed_moves) {
                Point player_pos_before_box{};
                // if we need to press 'W' to move the box, it means push position is right below it. Hence `.move(S)`.
                if (move == Move::W) {
                    player_pos_before_box = pushable_box.crate_pos.move(Move::S);
                } else if (move == Move::S) { // ...and so on.
                    player_pos_before_box = pushable_box.crate_pos.move(Move::W);
                } else if (move == Move::A) {
                    player_pos_before_box = pushable_box.crate_pos.move(Move::D);
                } else {
                    player_pos_before_box = pushable_box.crate_pos.move(Move::A);
                }

                // (2) check if push position near the crate is reachable
                std::optional<Path> o_path = std::nullopt;
                if (player_pos_before_box == state.player_pos()) { // player already near the crate, ready to push
                    o_path = Path(player_pos_before_box, player_pos_before_box);
                } else { // A*
                    o_path = Paths::plot_path(state.player_pos(), player_pos_before_box, state.f_adjacent_walkable());
                }

                if (o_path) {
                    reachable_push_positions.push_back(*o_path);
                    push_commands.push_back(move);
                }
            }
        }

        // combine found paths with push commands to create new states
        auto i_path = reachable_push_positions.begin();
        auto i_command = push_commands.begin();
        std::vector<NextState> next_states;
        next_states.reserve(reachable_push_positions.size());

        for (; i_path != reachable_push_positions.end() && i_command != push_commands.end(); ++i_path, ++i_command) {
            auto walk_commands = Paths::as_moves(*i_path);
            auto push_command = *i_command;

            GameState next_state = state;
            next_state.issue_orders(walk_commands);
            next_state.issue_order(push_command);

            std::vector<Move> moves = initial_moves;
            moves.insert(moves.end(), walk_commands.begin(), walk_commands.end());
            moves.push_back(push_command);

            next_states.emplace_back(next_state, moves);
        }

        // heuristic priority for states that have more boxes on targets
        std::sort(next_states.begin(), next_states.end());
        for (const auto& next : next_states) {
            auto result = solve(next.state, next.moves, states); // solve each (N+1)th state recursively
            if (!result.empty()) {
                return result;
            }
        }
        return {};
    }

    void prioritise_untargeted_boxes(std::vector<PushableBox>& boxes) {
        std::sort(boxes.begin(), boxes.end(), [&] (const auto& a, const auto &b) -> bool {
            bool a_ok = level.at(a.crate_pos)->type == CellType::TARGET;
            bool b_ok = level.at(b.crate_pos)->type == CellType::TARGET;
            if (a_ok == b_ok || !a_ok) {
                return true;
            }
            return false;
        });
    }

    bool is_unsolvable(const GameState& state) const {
        for (Point box : state.box_positions()) {
            if (auto o_cell = level.at(box); !o_cell || o_cell->type != CellType::TARGET) {
                if (is_unmovable_quad(box, state.box_positions())) {
                    return true;
                }
                if (is_locked_to_wall(box)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool is_unmovable_quad(Point box, const std::unordered_set<Point>& boxes) const {
        Point r = box.move(Move::D);
        Point d = box.move(Move::S);
        Point rd = box.move(Move::S).move(Move::D);
        bool is_quad = boxes.contains(r) && boxes.contains(d) && boxes.contains(rd);
        if (is_quad) {
            for (Point p : {box, r, d, rd}) {
                if (level.at(p)->type != CellType::TARGET) {
                    return true; // boxes form a 2x2 quad and at least one is not on target cell - that's a deadlock
                }
                return false;
            }
        }
        return false;
    }

    bool is_locked_to_wall(Point box) const {
        // Trying to detect stick-to-the-wall deadlocks. For example, the crate here is wasted:
        // #    x           #
        // ##################
        //
        // Note: if there is target point along the wall, we consider it winnable and do not fail. E.g.
        // #    x        .  #  <--- crate can be pushed to target, even though it's stuck with this path along the wall
        // ##################
        Point w = box.move(Move::W);
        Point a = box.move(Move::A);
        Point s = box.move(Move::S);
        Point d = box.move(Move::D);

        bool w_wall = level.at(w)->type == CellType::WALL;
        bool a_wall = level.at(a)->type == CellType::WALL;
        bool s_wall = level.at(s)->type == CellType::WALL;
        bool d_wall = level.at(d)->type == CellType::WALL;

        if (!w_wall && !a_wall && !s_wall && !d_wall) {
            return false;
        }

        if (w_wall && is_locked_to_wall(box, w)) {
            return true;
        }
        if (a_wall && is_locked_to_wall(box, a)) {
            return true;
        }
        if (s_wall && is_locked_to_wall(box, s)) {
            return true;
        }
        if (d_wall && is_locked_to_wall(box, d)) {
            return true;
        }
        return false;
    }

    bool is_locked_to_wall(Point box, Point wall) const {
        bool is_horizontal_lock = wall.y == box.y; // remember, `x` is actually vertical axis here T_T

        if (is_horizontal_lock) {
            int64_t wall_delta = static_cast<int64_t>(wall.x) - static_cast<int64_t>(box.x);

            // slide right
            for (size_t y = box.y + 1; y < level.dimensions().y; ++y) {
                Point box_level_pos { box.x, y };
                Point wall_level_pos { box.x + wall_delta, y };
                auto o_box_level_cell = level.at(box_level_pos);
                auto o_wall_level_cell = level.at(wall_level_pos);
                // #    x        .  #
                // ##################
                if (o_box_level_cell && o_box_level_cell->type == CellType::TARGET) {
                    return false; // solvable
                }

                // #   x #          #
                // ##################
                if (o_box_level_cell && o_box_level_cell->type == CellType::WALL) {
                    break; // potentially unsolvable lock; continue in another direction
                }

                // #    x           #
                // #########  #######
                if (o_wall_level_cell && o_wall_level_cell->type != CellType::WALL) {
                    return false; // solvable
                }
            }
            // slide left
            for (size_t y = box.y - 1; y >= 0; --y) {
                Point box_level_pos { box.x, y };
                Point wall_level_pos { box.x + wall_delta, y };
                auto o_box_level_cell = level.at(box_level_pos);
                auto o_wall_level_cell = level.at(wall_level_pos);
                // # .  x           #
                // ##################
                if (o_box_level_cell && o_box_level_cell->type == CellType::TARGET) {
                    return false; // solvable
                }

                // #   # x          #
                // ##################
                if (o_box_level_cell && o_box_level_cell->type == CellType::WALL) {
                    return true; // unsolvable
                }

                // #    x           #
                // ##  ##############
                if (o_wall_level_cell && o_wall_level_cell->type != CellType::WALL) {
                    return false; // solvable
                }
            }
            return false;
        } else { // vertical lock
            int64_t wall_delta = static_cast<int64_t>(wall.y) - static_cast<int64_t>(box.y);

            // slide down
            for (size_t x = box.x + 1; x < level.dimensions().x; ++x) {
                Point box_level_pos { x, box.y };
                Point wall_level_pos { x, box.y + wall_delta };
                auto o_box_level_cell = level.at(box_level_pos);
                auto o_wall_level_cell = level.at(wall_level_pos);
                // ##
                // #
                // #x
                // #
                // #.
                // ##
                if (o_box_level_cell && o_box_level_cell->type == CellType::TARGET) {
                    return false; // solvable
                }

                // ##
                // #
                // #x
                // #
                // #
                // ##
                if (o_box_level_cell && o_box_level_cell->type == CellType::WALL) {
                    break; // potentially unsolvable lock; continue in another direction
                }

                // ##
                // #
                // #x
                // #
                //
                // ##
                if (o_wall_level_cell && o_wall_level_cell->type != CellType::WALL) {
                    return false; // solvable
                }
            }
            // slide up
            for (size_t x = box.x - 1; x >= 0; --x) {
                Point box_level_pos { x, box.y };
                Point wall_level_pos { x, box.y + wall_delta };
                auto o_box_level_cell = level.at(box_level_pos);
                auto o_wall_level_cell = level.at(wall_level_pos);
                // ##
                // #.
                // #x
                // #
                // #
                // ##
                if (o_box_level_cell && o_box_level_cell->type == CellType::TARGET) {
                    return false; // solvable
                }

                // ##
                // #
                // #x
                // #
                // #
                // ##
                if (o_box_level_cell && o_box_level_cell->type == CellType::WALL) {
                    return true; // unsolvable
                }

                // ##
                //
                // #x
                // #
                // #
                // ##
                if (o_wall_level_cell && o_wall_level_cell->type != CellType::WALL) {
                    return false; // solvable
                }
            }
            return false;
        }
    }

    static bool validate_state_uniqueness(const GameState& state, NonIsomorphicStates& states) {
        auto reduced_state = state.reduced_state();
        std::vector<GameState>& states_with_same_box_positions = states[reduced_state];
        if (states_with_same_box_positions.empty()) {
            states_with_same_box_positions.push_back(state);
            return true; // state is unique, continue algorithm
        }

        for (const GameState& potentially_isomorphic_state : states_with_same_box_positions) {
            if (are_isomorphic(state, potentially_isomorphic_state)) {
                return false; // non unique state that differs only by mutually reachable player positions. Abort
            }
        }
        states_with_same_box_positions.push_back(state);
        return true; // we have some state with same box positions yet player position differs significantly. Continue
    }

    static bool are_isomorphic(const GameState& s1, const GameState& s2) {
        if (s1.player_pos() == s2.player_pos()) {
            return true;
        }

        return Paths::plot_path(s1.player_pos(), s2.player_pos(), s1.f_adjacent_walkable()).has_value();
    }
};


