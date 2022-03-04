#define CATCH_CONFIG_MAIN
#include "Catch2.hpp"

#include <game/GameState.hpp>
#include <logic/Paths.hpp>
#include <logic/Solver.hpp>

TEST_CASE("Path finding - path exists") {
    // x - box
    // # - wall
    // @ - initial player position
    // A,B - targets
    std::vector<std::string> map = {
        "####################",
        "#         x     B  #",
        "#     @   #  ##### #",
        "#         #        #",
        "#######   #####  ###",
        "#    Ax   #        #",
        "#  ####   #  #######",
        "#         #        #",
        "#                  #",
        "####################",
    };
    Level level(map);
    Point player_position {2, 6};
    Point goal_a {5, 5};
    Point goal_b {1, 16};
    GameState game(level, player_position, {{5, 6}, {1, 10}});
    std::string expected_moves_a = "sdssssaaaaawwddd";
    std::string expected_moves_b = "dddssssssddwwwddddwwdddwwaa";

    auto path_a = Paths::plot_path(player_position, goal_a, game.f_adjacent_walkable());
    auto path_b = Paths::plot_path(player_position, goal_b, game.f_adjacent_walkable());
    REQUIRE(path_a);
    REQUIRE(path_b);
    REQUIRE(Paths::as_string(*path_a) == expected_moves_a);
    REQUIRE(Paths::as_string(*path_b) == expected_moves_b);
}

TEST_CASE("Path finding - goal unreachable") {
    // x - box
    // # - wall
    // @ - initial player position
    // A,B - targets
    std::vector<std::string> map = {
            "####################",
            "#         x    #B  #",
            "#     @   #  #######",
            "#         #        #",
            "#######   #####  ###",
            "#   #Ax   #        #",
            "#  ####   #  #######",
            "#         #        #",
            "#                  #",
            "####################",
    };
    Level level(map);
    Point player_position {2, 6};
    Point goal_a {5, 5};
    Point goal_b {1, 16};
    GameState game(level, player_position, {{5, 6}, {1, 10}});

    auto path_a = Paths::plot_path(player_position, goal_a, game.f_adjacent_walkable());
    auto path_b = Paths::plot_path(player_position, goal_b, game.f_adjacent_walkable());
    REQUIRE(!path_a);
    REQUIRE(!path_b);
}

TEST_CASE("Path finding - trivials") {
    // x - box
    // # - wall
    // @ - initial player position
    // A,B - targets (target B is under player already)
    std::vector<std::string> map = {
            "####################",
            "#         x        #",
            "#     @   #  ##### #",
            "#     A   #        #",
            "#######   #####  ###",
            "#     x   #        #",
            "#  ####   #  #######",
            "#         #        #",
            "#                  #",
            "####################",
    };
    Level level(map);
    Point player_position {2, 6};
    Point goal_a {3, 6};
    Point goal_b  = player_position;
    GameState game(level, player_position, {{5, 6}, {1, 10}});

    auto path_a = Paths::plot_path(player_position, goal_a, game.f_adjacent_walkable());
    auto path_b = Paths::plot_path(player_position, goal_b, game.f_adjacent_walkable());
    REQUIRE(path_a);
    REQUIRE(path_b);
    REQUIRE(Paths::as_string(*path_a) == "s");
    REQUIRE(Paths::as_moves(*path_b).empty());
}

TEST_CASE("Path finding - out of boundaries") {
    // x - box
    // # - wall
    // @ - initial player position
    // target is somewhere in outer space
    std::vector<std::string> map = {
            "####################",
            "#         x        #",
            "#     @   #  ##### #",
            "#         #        #",
            "#######   #####  ###",
            "#     x   #        #",
            "#  ####   #  #######",     //    A ------> 60
            "#         #        #",     //    |
            "#                  #",     // 30 |
            "####################",     //    V
    };
    Level level(map);
    Point player_position {2, 6};
    Point goal {30, 60};
    GameState game(level, player_position, {{5, 6}, {1, 10}});

    auto path = Paths::plot_path(player_position, goal, game.f_adjacent_walkable());
    REQUIRE(!path);
}

TEST_CASE("Solving - trivial straight line") {
    std::vector<std::string> map = {
            "###",
            "#@#",
            "# #",
            "# #",
            "#x#",
            "# #",
            "# #",
            "# #",
            "#.#",
            "###",
    };
    Level level(map);
    Point player_position {1, 1};
    GameState game(level, player_position, {{4, 1}});
    std::string expected_moves = "ssssss";

    Solver solver(level);
    auto solution = solver.solve(game);
    REQUIRE(Paths::as_string(solution) == expected_moves);
}

TEST_CASE("Solving - trivial no solution") {
    std::vector<std::string> map = {
            "###",
            "#@#",
            "# #",
            "# #",
            "#x#",
            "# #",
            "# #",
            "###",
            "#.#",
            "###",
    };
    Level level(map);
    Point player_position {1, 1};
    GameState game(level, player_position, {{4, 1}});
    std::string expected_moves = "";

    Solver solver(level);
    auto solution = solver.solve(game);
    REQUIRE(Paths::as_string(solution) == expected_moves);
}

TEST_CASE("Solving - trivial straight line two boxes") {
    std::vector<std::string> map = {
            "###",
            "#.#",
            "# #",
            "#x#",
            "#@#",
            "# #",
            "#x#",
            "# #",
            "#.#",
            "###",
    };
    Level level(map);
    Point player_position {4, 1};
    GameState game(level, player_position, {{3, 1}, {6, 1}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - trivial straight line unreachable box") {
    std::vector<std::string> map = {
            "###",
            "#x#",
            "#.#",
            "# #",
            "#@#",
            "# #",
            "# #",
            "# #",
            "# #",
            "###",
    };
    Level level(map);
    Point player_position {4, 1};
    GameState game(level, player_position, {{1, 1}});

    std::string expected_moves = "";

    Solver solver(level);
    auto solution = solver.solve(game);
    REQUIRE(Paths::as_string(solution) == expected_moves);
}

TEST_CASE("Solving - trivial straight line pushable box no solution") {
    std::vector<std::string> map = {
            "###",
            "# #",
            "#x#",
            "#.#",
            "#@#",
            "# #",
            "# #",
            "# #",
            "# #",
            "###",
    };
    Level level(map);
    Point player_position {4, 1};
    GameState game(level, player_position, {{2, 1}});

    std::string expected_moves = "";

    Solver solver(level);
    auto solution = solver.solve(game);
    REQUIRE(Paths::as_string(solution) == expected_moves);
}

TEST_CASE("Solving - trivial straight line two boxes horizontal") {
    std::vector<std::string> map = {
            "##############",
            "# . x   @  x.#",
            "##############",
    };
    Level level(map);
    Point player_position {1, 8};
    GameState game(level, player_position, {{1, 4}, {1, 11}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - retractable box") {
    std::vector<std::string> map = {
            "####",
            "#@.#",
            "#  #",
            "#  #",
            "#  #",
            "##x#",
            "#  #",
            "#  #",
            "#  #",
            "####",
    };
    Level level(map);
    Point player_position {1, 1};
    GameState game(level, player_position, {{5, 2}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - retractable box, two boxes") {
    std::vector<std::string> map = {
            "####",
            "#@.#",
            "#. #",
            "#x #",
            "#  #",
            "##x#",
            "#  #",
            "#  #",
            "#  #",
            "####",
    };
    Level level(map);
    Point player_position {1, 1};
    GameState game(level, player_position, {{5, 2}, {3, 1}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - moving box around") {
    std::vector<std::string> map = {
            "########",
            "####  ##",
            "#     ##",
            "#@x#  .#",
            "#  #####",
            "########",
    };
    Level level(map);
    Point player_position {3, 1};
    GameState game(level, player_position, {{3, 2}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level 1 box") {
    std::vector<std::string> map = {
            "##############",
            "########  ####",
            "#          ###",
            "# @x  ##     #",
            "#      ##   .#",
            "#         ####",
            "##############",
    };
    Level level(map);
    Point player_position {3, 2};
    GameState game(level, player_position, {{3, 3}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level 2 boxes") {
    std::vector<std::string> map = {
            "##############",
            "########  ####",
            "#          ###",
            "# @xx ##     #",
            "#      ##  ..#",
            "#         ####",
            "##############",
    };
    Level level(map);
    Point player_position {3, 2};
    GameState game(level, player_position, {{3, 3}, {3, 4}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level 3 boxes") {
    std::vector<std::string> map = {
            "##############",
            "########  ####",
            "#          ###",
            "# @xx ##   ..#",
            "#  x   ##   .#",
            "#         ####",
            "##############",
    };
    Level level(map);
    Point player_position {3, 2};
    GameState game(level, player_position, {{3, 3}, {3, 4}, {4, 3}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level 3 boxes (variation)") {
    std::vector<std::string> map = {
            "##############",
            "########  ####",
            "#          ###",
            "# @xx ##    .#",
            "# x    ##  ..#",
            "#         ####",
            "##############",
    };
    Level level(map);
    Point player_position {3, 2};
    GameState game(level, player_position, {{3, 3}, {3, 4}, {4, 2}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level 4 boxes") {
    std::vector<std::string> map = {
            "##############",
            "########  ####",
            "#          ###",
            "# @xx ##   ..#",
            "# xx   ##  ..#",
            "#         ####",
            "##############",
    };
    Level level(map);
    Point player_position {3, 2};
    GameState game(level, player_position, {{3, 3}, {3, 4}, {4, 2}, {4, 3}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level canonical") {
    std::vector<std::string> map = {
            "########",
            "###   ##",
            "#.    ##",
            "###  .##",
            "#.##  ##",
            "# # . ##",
            "#  .  .#",
            "#   .  #",
            "########",
    };
    Level level(map);
    Point player_position {2, 2};
    GameState game(level, player_position, {{2, 3},
                                            {3, 4},
                                            {4, 4},
                                            {6, 1},
                                            {6, 3},
                                            {6, 4},
                                            {6, 5}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}

TEST_CASE("Solving - real level, trivial solution, many crates") {
    std::vector<std::string> map = {
        "##########",
        "# .......#",
        "#        #",
        "#        #",
        "# .......#",
        "#        #",
        "#        #",
        "##########",
    };
    Level level(map);
    Point player_position {1, 1};
    GameState game(level, player_position, {
        {2, 2},
        {2, 3},
        {2, 4},
        {2, 5},
        {2, 6},
        {2, 7},
        {2, 8},
        {5, 2},
        {5, 3},
        {5, 4},
        {5, 5},
        {5, 6},
        {5, 7},
        {5, 8}});

    Solver solver(level);
    auto solution = solver.solve(game);
    game.issue_orders(solution);
    REQUIRE(game.is_victory());
}