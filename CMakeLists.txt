cmake_minimum_required(VERSION 3.17)
project(sokoban)

set(CMAKE_CXX_STANDARD 20)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})


add_executable(sokoban main.cpp src/game/Level.hpp src/util/FileUtil.hpp src/game/GameState.hpp src/logic/Paths.hpp src/logic/Solver.hpp)
target_link_libraries(sokoban ${CURSES_LIBRARIES})

enable_testing()
add_subdirectory(test)