cmake_minimum_required(VERSION 3.16)
project(pacman_reimagined_cpp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Warnings
if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  # Default warning set — kept lean so CI stays green. Use
  # `cmake -DSTRICT_WARNINGS=ON` to add -Wshadow -Wold-style-cast
  # -Wdouble-promotion etc. when auditing.
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter
                      -Wnull-dereference -Wunused -Wformat=2
                      -Wno-missing-field-initializers)
  if(STRICT_WARNINGS)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
                        $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
                        $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
                        $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
                        $<$<COMPILE_LANGUAGE:CXX>:-Wdouble-promotion>
                        $<$<COMPILE_LANGUAGE:CXX>:-Wcast-align>)
  endif()
endif()

# --- SDL2 ---
find_package(SDL2 REQUIRED)
# Bitmap font in render.cpp covers HUD; no SDL2_ttf dep required.

# --- target ---
set(PACMAN_SRC
  src/glibc_compat.c
  src/main.cpp
  src/app.cpp
  src/audio.cpp
  src/maze.cpp
  src/pacman.cpp
  src/ghost.cpp
  src/score.cpp
  src/powerup_manager.cpp
  src/input_router.cpp
  src/controller_manager.cpp
  src/scenes/menu.cpp
  src/scenes/mode_select.cpp
  src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp
  src/modes/alternating.cpp
  src/modes/coop.cpp
  src/ui/render.cpp
  src/ui/hud_dual.cpp
  src/ui/turn_swap.cpp
)

add_executable(pacman-reimagined ${PACMAN_SRC})

target_include_directories(pacman-reimagined
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(pacman-reimagined
  PRIVATE
    ${SDL2_LIBRARIES}
)

# Static-link libstdc++ and libgcc to free the binary from the host's
# libstdc++ ABI (EmuELEC v4.6 ships libstdc++ 6.0.30 / GLIBCXX_3.4.30).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-reimagined PRIVATE
    -static-libstdc++
    -static-libgcc
  )
endif()

# Strip in release
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-reimagined PRIVATE -s)
endif()

install(TARGETS pacman-reimagined DESTINATION bin)
install(DIRECTORY assets DESTINATION share/pacman-reimagined)

# --- Tests: headless game-logic unit tests ---
add_executable(pacman-tests
  src/glibc_compat.c
  src/maze.cpp
  src/pacman.cpp
  src/ghost.cpp
  src/score.cpp
  src/powerup_manager.cpp
  tests/test_game.cpp
)
target_include_directories(pacman-tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(pacman-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
enable_testing()
add_test(NAME game_logic COMMAND pacman-tests)

# Sim tests need full scene + render code (drives MultiGameplay headlessly).
add_executable(pacman-sim-tests
  src/glibc_compat.c
  src/app.cpp
  src/audio.cpp
  src/maze.cpp
  src/pacman.cpp
  src/ghost.cpp
  src/score.cpp
  src/powerup_manager.cpp
  src/input_router.cpp
  src/controller_manager.cpp
  src/scenes/menu.cpp
  src/scenes/mode_select.cpp
  src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp
  src/modes/alternating.cpp
  src/modes/coop.cpp
  src/ui/render.cpp
  src/ui/hud_dual.cpp
  src/ui/turn_swap.cpp
  tests/test_simulation.cpp
)
target_include_directories(pacman-sim-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-sim-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-sim-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME simulation COMMAND pacman-sim-tests)

# Fuzz test: random-input long run with invariant checks
add_executable(pacman-fuzz-tests
  src/glibc_compat.c
  src/app.cpp
  src/audio.cpp
  src/maze.cpp
  src/pacman.cpp
  src/ghost.cpp
  src/score.cpp
  src/powerup_manager.cpp
  src/input_router.cpp
  src/controller_manager.cpp
  src/scenes/menu.cpp
  src/scenes/mode_select.cpp
  src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp
  src/modes/alternating.cpp
  src/modes/coop.cpp
  src/ui/render.cpp
  src/ui/hud_dual.cpp
  src/ui/turn_swap.cpp
  tests/test_fuzz.cpp
)
target_include_directories(pacman-fuzz-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-fuzz-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-fuzz-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME fuzz COMMAND pacman-fuzz-tests)

# Hot-plug fuzz — small surface, just controller_manager + input_router
add_executable(pacman-hotplug-tests
  src/glibc_compat.c
  src/controller_manager.cpp
  src/input_router.cpp
  tests/test_hotplug.cpp
)
target_include_directories(pacman-hotplug-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-hotplug-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-hotplug-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME hotplug COMMAND pacman-hotplug-tests)

# Level-completion + game-over flow tests
add_executable(pacman-level-tests
  src/glibc_compat.c
  src/app.cpp
  src/audio.cpp
  src/maze.cpp
  src/pacman.cpp
  src/ghost.cpp
  src/score.cpp
  src/powerup_manager.cpp
  src/input_router.cpp
  src/controller_manager.cpp
  src/scenes/menu.cpp
  src/scenes/mode_select.cpp
  src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp
  src/modes/alternating.cpp
  src/modes/coop.cpp
  src/ui/render.cpp
  src/ui/hud_dual.cpp
  src/ui/turn_swap.cpp
  tests/test_level_clear.cpp
)
target_include_directories(pacman-level-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-level-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-level-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME level_clear COMMAND pacman-level-tests)

# AI playthrough — greedy bot
add_executable(pacman-ai-tests
  src/glibc_compat.c src/app.cpp src/audio.cpp src/maze.cpp src/pacman.cpp src/ghost.cpp
  src/score.cpp src/powerup_manager.cpp src/input_router.cpp src/controller_manager.cpp
  src/scenes/menu.cpp src/scenes/mode_select.cpp src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp src/modes/alternating.cpp src/modes/coop.cpp
  src/ui/render.cpp src/ui/hud_dual.cpp src/ui/turn_swap.cpp
  tests/test_ai_playthrough.cpp
)
target_include_directories(pacman-ai-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-ai-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-ai-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME ai_playthrough COMMAND pacman-ai-tests)

# Regression suite — codified past user complaints
add_executable(pacman-regression-tests
  src/glibc_compat.c src/app.cpp src/audio.cpp src/maze.cpp src/pacman.cpp src/ghost.cpp
  src/score.cpp src/powerup_manager.cpp src/input_router.cpp src/controller_manager.cpp
  src/scenes/menu.cpp src/scenes/mode_select.cpp src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp src/modes/alternating.cpp src/modes/coop.cpp
  src/ui/render.cpp src/ui/hud_dual.cpp src/ui/turn_swap.cpp
  tests/test_regression.cpp
)
target_include_directories(pacman-regression-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-regression-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-regression-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME regression COMMAND pacman-regression-tests)

# Perf budget
add_executable(pacman-perf-tests
  src/glibc_compat.c src/app.cpp src/audio.cpp src/maze.cpp src/pacman.cpp src/ghost.cpp
  src/score.cpp src/powerup_manager.cpp src/input_router.cpp src/controller_manager.cpp
  src/scenes/menu.cpp src/scenes/mode_select.cpp src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp src/modes/alternating.cpp src/modes/coop.cpp
  src/ui/render.cpp src/ui/hud_dual.cpp src/ui/turn_swap.cpp
  tests/test_perf.cpp
)
target_include_directories(pacman-perf-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-perf-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-perf-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME perf_budget COMMAND pacman-perf-tests)

# Per-ghost AI targeting
add_executable(pacman-ghost-ai-tests
  src/glibc_compat.c src/maze.cpp src/pacman.cpp src/ghost.cpp src/score.cpp
  src/powerup_manager.cpp tests/test_ghost_ai.cpp)
target_include_directories(pacman-ghost-ai-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-ghost-ai-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-ghost-ai-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME ghost_ai COMMAND pacman-ghost-ai-tests)

# Power-ups + score thresholds
add_executable(pacman-powerup-tests
  src/glibc_compat.c src/maze.cpp src/score.cpp src/powerup_manager.cpp
  tests/test_powerups.cpp)
target_include_directories(pacman-powerup-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-powerup-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-powerup-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME powerups COMMAND pacman-powerup-tests)

# Coop-specific (ghost retarget, power-up scope, rescue revive)
add_executable(pacman-coop-tests
  src/glibc_compat.c src/app.cpp src/audio.cpp src/maze.cpp src/pacman.cpp src/ghost.cpp
  src/score.cpp src/powerup_manager.cpp src/input_router.cpp src/controller_manager.cpp
  src/scenes/menu.cpp src/scenes/mode_select.cpp src/scenes/multi_gameplay.cpp
  src/modes/solo.cpp src/modes/alternating.cpp src/modes/coop.cpp
  src/ui/render.cpp src/ui/hud_dual.cpp src/ui/turn_swap.cpp
  tests/test_coop_logic.cpp)
target_include_directories(pacman-coop-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-coop-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-coop-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME coop_logic COMMAND pacman-coop-tests)

# Maze + pacman edge cases
add_executable(pacman-edge-tests
  src/glibc_compat.c src/maze.cpp src/pacman.cpp src/score.cpp src/powerup_manager.cpp
  tests/test_maze_edges.cpp)
target_include_directories(pacman-edge-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})
target_link_libraries(pacman-edge-tests PRIVATE ${SDL2_LIBRARIES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(pacman-edge-tests PRIVATE -static-libstdc++ -static-libgcc)
endif()
add_test(NAME maze_edges COMMAND pacman-edge-tests)
