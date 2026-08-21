cmake_minimum_required(VERSION 3.16)
project(minicore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(core STATIC
    src/helpers/color.cpp
    src/helpers/util.cpp
    src/helpers/config.h
)
target_include_directories(core PUBLIC src/helpers)

# toml++ (header-only)
add_library(toml INTERFACE)
target_include_directories(toml INTERFACE external/tomlplusplus/include)

# Готовые инструменты
add_executable(sfetch src/sfetch/main.cpp)
target_link_libraries(sfetch PRIVATE core toml)
add_executable(scat src/cat/scat.cpp)
target_link_libraries(scat PRIVATE core)

# Заглушки (пока без main)
# add_executable(s-kill src/s-kill/main.cpp)
# add_executable(sls src/simple/ls.cpp)

# Сборка: cmake -B build && cmake --build build
# Бинарники: build/sfetch, build/scat
