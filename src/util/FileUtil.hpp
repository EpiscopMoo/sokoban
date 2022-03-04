#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <variant>

struct SokobanParseResult {
    std::vector<std::string> level;
    Point player_position;
    std::vector<Point> box_positions;
};

using ErrorMessage = std::string;

class FileUtil {
public:
    FileUtil() = delete;
    static std::variant<SokobanParseResult, ErrorMessage> read_file(const std::string& filename) {
        std::optional<Point> o_player_position = std::nullopt;
        std::vector<std::string> result;
        std::vector<Point> boxes;

        std::ifstream file;
        file.exceptions(std::ifstream::badbit);

        try {
            file.open(filename, std::fstream::in);

            if (!file.is_open()) {
                throw std::exception();
            }

            size_t x = 0;
            for(std::string line; getline(file, line);) {
                if (!line.empty()) {

                    std::string level_row;
                    level_row.reserve(line.size());

                    size_t y = 0;
                    for (char c : line) {
                        if (c == '@') { // Player position
                            if (o_player_position) {
                                throw std::invalid_argument("More than one player position specified");
                            }
                            o_player_position = Point{x, y};
                            level_row += ' ';
                        } else if (c == 'x') { // crate
                            boxes.push_back(Point{x, y});
                            level_row += ' ';
                        } else if (c == 'X') { // crate standing on target point
                            boxes.push_back(Point{x, y});
                            level_row += '.';
                        } else {
                            level_row += c;
                        }
                        ++y;
                    }
                    result.push_back(level_row);
                }
                ++x;
            }
            sanity_check(o_player_position, result, boxes);
            return SokobanParseResult{result, *o_player_position, boxes};
        } catch (const std::exception& ex) {
            return "Could not open file " + filename + ", " + std::string(ex.what());
        }
    }
private:
    static void sanity_check(std::optional<Point> o_player_position,
                             const std::vector<std::string>& layout,
                             const std::vector<Point>& boxes) {
        if (!o_player_position) {
            throw std::invalid_argument("No player position found");
        }
        size_t height = layout.size();
        if (height <= 3) {
            throw std::invalid_argument("Level has too few rows");
        }

        size_t width = layout[0].size();
        if (width <= 3) {
            throw std::invalid_argument("Level has too few columns");
        }

        for (const auto& row : layout) {
            if (row.length() != width) {
                throw std::invalid_argument("Level rows differ in length");
            }
            char border_left = row.front();
            char border_right = row.back();
            if (border_left != '#' || border_right != '#') {
                throw std::invalid_argument("Invalid level borders");
            }
        }

        for (size_t col = 0; col < width; ++col) {
            char border_top = layout.front()[col];
            char border_bot = layout.back()[col];
            if (border_top != '#' || border_bot != '#') {
                throw std::invalid_argument("Invalid level borders");
            }
        }

        if (o_player_position->x == 0 || o_player_position->x >= height ||
            o_player_position->y == 0 || o_player_position->y >= width) {
            throw std::invalid_argument("Player position is out of bounds of playable area");
        }
    }
};


