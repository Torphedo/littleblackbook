cmake_minimum_required(VERSION 3.13)
project(blackbook
    VERSION 1.0.0
    LANGUAGES CXX C
)
# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(
    lib
    lib/nfde
    lib/imgui
    lib/bobtail
    lib/raudio/src
    ${blackbook}/src
)

# Add ImGui as a library
# add_library(imgui STATIC
#     lib/imgui/imgui.cpp
#     lib/imgui/imgui_demo.cpp
#     lib/imgui/imgui_draw.cpp
#     lib/imgui/imgui_tables.cpp
#     lib/imgui/imgui_widgets.cpp
#     lib/imgui/backends/imgui_impl_opengl3.cpp
#     lib/imgui/backends/imgui_impl_glfw.cpp
# )

# Dependencies
add_subdirectory(lib/bobtail)
add_subdirectory(lib/raudio/projects/CMake)
add_subdirectory(lib/nfde)

set(BUILD_STATIC ON)
set(BUILD_SHARED OFF)
set(BUILD_SHELL OFF)
set(BUILD_STATIC_SHELL OFF)
add_subdirectory(lib/sqlite)

add_executable(blackbook
    src/main.cxx
    src/id3.cxx
    src/song.cxx
    src/tags.cxx
    src/sqlgen.cxx

    src/arguments.cxx
    src/scope_timer.cxx
)

target_link_libraries(blackbook PRIVATE bobtail nfd raudio sqlite3-static)
