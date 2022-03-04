#pragma once
#include <queue>
#include <optional>
#include <functional>
#include <unordered_set>
#include <list>

struct Path {
    Point goal;
    std::list<Point> points;

    Path(Point _goal, Point p) : goal(_goal) {
        points.push_back(p);
    }

    Path(Point _goal, const Path& path, Point p) : goal(_goal) {
        points = path.points;
        points.push_back(p);
    }

    bool operator<(const Path& other) const {
        // inverse check due to c++ heap-queue behaviour
        size_t my_dist = Point::distance(points.back(), goal);
        size_t other_dist = Point::distance(other.points.back(), goal);
        if (my_dist == other_dist) {
            return points.size() > other.points.size();
        }
        return Point::distance(points.back(), goal) > Point::distance(other.points.back(), goal);
    }

    Point last() {
        return  points.back();
    }
};

class Paths {
public:
    Paths() = delete;
    static std::optional<Path> plot_path(Point start,
                                         Point goal,
                                         std::function<std::vector<Point>(Point)> adjacent_getter) {
        std::unordered_set<Point> visited;
        std::priority_queue<Path> paths;
        paths.push(Path(goal, start));

        while (!paths.empty()) {
            Path best_path = paths.top();
            paths.pop();

            Point current = best_path.last();
            if (!visited.count(current)) {
                if (current == goal) {
                    return best_path;
                }

                visited.insert(current);
                for (Point adjacent_pt : adjacent_getter(current)) {
                    if (!visited.count(adjacent_pt)) {
                        paths.push(Path(goal, best_path, adjacent_pt));
                    }
                }
            }
        }
        return std::nullopt;
    }

    static std::vector<Move> as_moves(const Path& path) {
        if (path.points.size() < 2) {
            return {};
        }
        std::vector<Move> moves;
        moves.reserve(path.points.size());
        auto iter = path.points.begin();
        auto next = ++path.points.begin();
        for (; next != path.points.end(); ++iter, ++next) {
            moves.push_back(move_between(*iter, *next));
        }
        return moves;
    }

    static std::vector<Move> as_moves(const std::string& str) {
        std::vector<Move> moves;
        moves.reserve(str.length());
        for (auto c : str) {
            moves.push_back(move_of(c));
        }
        return moves;
    }

    static std::string as_string(const std::vector<Move>& moves) {
        std::string result;
        for (Move move : moves) {
            switch (move) {
                case Move::W:
                    result += 'w';
                    break;
                case Move::A:
                    result += 'a';
                    break;
                case Move::S:
                    result += 's';
                    break;
                case Move::D:
                    result += 'd';
                    break;
                default:
                    break;
            }
        }
        return result;
    }

    static std::string as_string(const Path& path) {
        return as_string(as_moves(path));
    }
private:
    static Move move_between(Point p1, Point p2) {
        // remember that x is actually vertical axis in curses notation
        size_t v_diff = p2.x - p1.x;
        size_t h_diff = p2.y - p1.y;
        if (h_diff == 0) {
            if (v_diff == 1) {
                return Move::S;
            }
            if (v_diff == -1) {
                return Move::W;
            }
        }
        if (v_diff == 0) {
            if (h_diff == 1) {
                return Move::D;
            }
            if (h_diff == -1) {
                return Move::A;
            }
        }
        return Move::NONE;
    }
};


