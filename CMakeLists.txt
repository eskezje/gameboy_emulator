cmake_minimum_required(VERSION 3.20)
project(gameboy_emulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
# for my nvim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

add_executable(gameboy_emulator
    src/main.cpp
    src/cart.cpp
    src/cpu.cpp
    src/cpu_instructions.cpp
    src/emulator_core.cpp
    src/memory_bus.cpp
)

target_include_directories(gameboy_emulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gameboy_emulator PRIVATE SDL3::SDL3)
