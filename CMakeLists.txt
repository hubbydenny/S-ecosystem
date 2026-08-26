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


add_executable(sfetch src/sfetch/main.cpp)
target_link_libraries(sfetch PRIVATE core toml)
add_executable(scat src/cat/scat.cpp)
target_link_libraries(scat PRIVATE core)

add_executable(sk src/s-kill/main.cpp)
target_link_libraries(sk PRIVATE core)

add_executable(scopy src/another/copy.cpp)

add_executable(space src/another/space.cpp)

add_executable(srm src/another/deleter.cpp)

add_executable(smv src/another/renamernsizer.cpp)

add_executable(se src/shell/main.cpp)


# add_executable(s-kill src/s-kill/main.cpp)
# add_executable(sls src/simple/ls.cpp)

# Сборка: cmake -B build && cmake --build build
# Бинарники: build/sfetch, build/scat
