#include "game/Level.hpp"
#include "game/GameState.hpp"
#include "logic/Solver.hpp"
#include "util/FileUtil.hpp"

#include <numeric>
#include <iostream>
#include <filesystem>
#include <memory>
#include <chrono>
#include <iomanip>

using PLevel = std::shared_ptr<Level>;
using PGameState = std::shared_ptr<GameState>;
using PSolver = std::shared_ptr<Solver>;

constexpr int DEFAULT_ITERATIONS = 100;
constexpr int MAX_ITERATIONS = 100000;

void print_stats(const std::vector<std::vector<uint64_t>>& measures) {
    std::cout << std::fixed;
    std::cout << std::setprecision(2);
    std::cout << "Level #\tAvg.\t\tTotal" << std::endl;

    size_t level_count = measures[0].size();
    std::vector<uint64_t> total_per_iteration;
    total_per_iteration.reserve(measures.size());

    for (const auto& level_measures : measures) {
        total_per_iteration.push_back(std::reduce(level_measures.begin(), level_measures.end()));
    }

    uint64_t total = std::reduce(total_per_iteration.begin(), total_per_iteration.end());
    double average = static_cast<double>(total) / static_cast<double>(measures.size());

    for (size_t level = 0; level < level_count; ++level) {
        uint64_t total_per_level = 0;
        for (size_t iteration = 0; iteration < measures.size(); ++iteration) {
            total_per_level += measures[iteration][level];
        }
        double average_per_level = static_cast<double>(total_per_level) / static_cast<double>(measures.size());
        std::cout << level << "\t\t" << average_per_level << "\t\t" << total_per_level << "\n";
    }
    std::cout << "\n";
    std::cout << "Avg. iteration time " << average << " ms\n";
    std::cout << "Total               " << total << " ms" << std::endl;
}

void run_benchmark(int iterations, const std::vector<PGameState>& states, const std::vector<PSolver>& solvers) {
    namespace t = std::chrono;
    size_t level_count = states.size();
    std::vector<std::vector<uint64_t>> measures(iterations, std::vector<uint64_t>(level_count, 0));
    std::cout << "Running";
    for (int i = 0; i < iterations; ++i) {
        std::cout << ".";
        std::cout.flush();
        for (int j = 0; j < level_count; ++j) {
            auto p_state = states[j];
            auto p_solver = solvers[j];
            auto start = t::steady_clock::now();
            if (p_solver->solve(*p_state).empty()) {
                throw std::logic_error("Unsolvable level encountered: #" + std::to_string(j));
            }
            uint64_t millis = t::duration_cast<t::milliseconds>(t::steady_clock::now() - start).count();
            measures[i][j] = millis;
        }
    }
    std::cout << std::endl;
    print_stats(measures);
    std::cout << "Done." << std::endl;
}

int main(int argc, const char** argv) {
    namespace fs = std::filesystem;

    if (argc <= 1) {
        std::cout << "Please provide path to directory with Sokoban levels as first argument." << std::endl;
        std::cin.get();
        return 0;
    }

    const char* directory_c = argv[1];
    std::string directory(directory_c);
    std::vector<std::string> filenames;
    for (const auto& fs_entry : fs::directory_iterator(directory)) {
        filenames.push_back(fs_entry.path());
    }
    std::sort(filenames.begin(), filenames.end());

    std::vector<PLevel> levels;
    std::vector<PGameState> states;
    std::vector<PSolver> solvers;

    for (const auto& filename : filenames) {
        auto v_parse_result = FileUtil::read_file(filename);
        if (std::holds_alternative<ErrorMessage>(v_parse_result)) {
            std::cout << "Error: " << std::get<ErrorMessage>(v_parse_result) << std::endl;
            std::cin.get();
            return 0;
        }

        SokobanParseResult parsed_sokoban = std::get<SokobanParseResult>(v_parse_result);
        levels.push_back(std::make_shared<Level>(parsed_sokoban.level));
        states.push_back(std::make_shared<GameState>(*levels.back(), parsed_sokoban.player_position, parsed_sokoban.box_positions));
        solvers.push_back(std::make_shared<Solver>(*levels.back()));
    }

    int iterations = DEFAULT_ITERATIONS;
    if (argc > 2) {
        iterations = atoi(argv[2]);
        if (iterations <= 0 || iterations > MAX_ITERATIONS) {
            std::cout << "Invalid amount of iterations given: " << iterations << std::endl;
            std::cin.get();
            return 0;
        }
    }

    run_benchmark(iterations, states, solvers);

    solvers.clear();
    states.clear();
    levels.clear();
    return 0;
}
