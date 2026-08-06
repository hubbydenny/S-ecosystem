cmake_minimum_required(VERSION 3.16)
project(minicore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Общая библиотека: цвета ANSI, чтение /proc, терминальные утилиты.
# Каждая утилита линкуется только с ней — ничего внешнего.
add_library(core STATIC
    src/common/color.cpp
    src/common/sysinfo.cpp
    src/common/util.cpp
)
target_include_directories(core PUBLIC src/common)

# Четыре инструмента мини-экосистемы
foreach(tool fastfetch ls kill cat)
    add_executable(${tool} src/${tool}/main.cpp)
    target_link_libraries(${tool} PRIVATE core)
endforeach()

# Сборка: cmake -B build && cmake --build build
# Бинарники: build/fastfetch, build/ls, build/kill, build/cat
