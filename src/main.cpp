#include "game/Level.hpp"
#include "game/GameState.hpp"
#include "game/InterLayer.hpp"
#include "logic/Solver.hpp"
#include "util/FileUtil.hpp"

#include <iostream>

int main(int argc, const char** argv) {
    if (argc <= 1) {
        std::cout << "Please provide path to file with Sokoban level as first argument.\n"
                     "Pass 'auto' as second argument if you wish to solve the game automatically." << std::endl;
        std::cin.get();
        return 0;
    }

    const char* path_c = argv[1];
    std::string path(path_c);

    auto v_parse_result = FileUtil::read_file(path);

    if (std::holds_alternative<ErrorMessage>(v_parse_result)) {
        std::cout << "Error: " << std::get<ErrorMessage>(v_parse_result) << std::endl;
        std::cin.get();
        return 0;
    }

    SokobanParseResult parsed_sokoban = std::get<SokobanParseResult>(v_parse_result);

    Level level(parsed_sokoban.level);
    GameState game(level, parsed_sokoban.player_position, parsed_sokoban.box_positions);

    bool manual = true;
    if (argc > 2 && std::string(argv[2]) == "auto") { // mode
        manual = false;
    }

    InterLayer interLayer(game);

    interLayer.init_screen();
    if (manual) {
        interLayer.manual_loop();
    } else {
        Solver solver(level);
        interLayer.execute_commands(solver.solve(game));
    }
    return 0;
}
