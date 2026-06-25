# Pokémon Red/Blue game-specific build extension.
#
# Included by the generated project's CMakeLists.txt via the agnostic
# GBRECOMP game_build.cmake hook (emitted by the recompiler). GBRECOMP_GAME_TARGET
# is the executable target. Adds the relocated Pokémon mock modules + the C++
# overlay TU (game_draw_overlay / game_on_frame), and the mock header dir.
# The ImGui and runtime include dirs come transitively from the gbrt target.

target_sources(${GBRECOMP_GAME_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/pokemon/mock_gen1.c
    ${CMAKE_CURRENT_LIST_DIR}/pokemon/mock_gen2.c
    ${CMAKE_CURRENT_LIST_DIR}/pokemon/mock_ir.c
    ${CMAKE_CURRENT_LIST_DIR}/pokemon/mock_inject_file.c
    ${CMAKE_CURRENT_LIST_DIR}/pokemon/mock_evolve_patch.c
    ${CMAKE_CURRENT_LIST_DIR}/pokemon/mock_wild_5050.c
    ${CMAKE_CURRENT_LIST_DIR}/pokemon_overlay.cpp
)

target_include_directories(${GBRECOMP_GAME_TARGET} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

message(STATUS "${GBRECOMP_GAME_TARGET}: Pokémon mocks + overlay wired via game_build.cmake")
