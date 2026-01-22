cmake_minimum_required(VERSION 3.13)

project(codecrafters-shell C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.c)

add_executable(shell ${SOURCE_FILES})

target_include_directories(shell PRIVATE
    src
)

target_link_libraries(shell PRIVATE readline)
