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
    src
    lib/bobtail

    # We don't actually need this, but bobtail::setup_opengl() relies on input.h,
    # which uses vector types.
    lib/cglm/include
    lib/imgui
    lib/glad/include
    lib/glfw/include
    lib/raudio/src
    lib/nfde/src/include
)

# Add ImGui as a library
add_library(imgui STATIC
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
    lib/imgui/misc/cpp/imgui_stdlib.cpp
)

# Dependencies
set(BOBTAIL_OPENGL ON)
add_subdirectory(lib/bobtail)
add_subdirectory(lib/raudio/projects/CMake)
add_subdirectory(lib/nfde)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(lib/glfw)

# We turn off as many SQLite options as we can to reduce compile time
set(ENABLE_STATIC ON)
set(ENABLE_SHARED OFF)
set(BUILD_SHELL OFF)
set(ENABLE_STATIC_SHELL ON)
set(ENABLE_FTS4 OFF) # We only use FTS5
set(ENABLE_RTREE OFF)
set(OMIT_JSON OFF)
set(OMIT_AUTOINIT OFF)
add_subdirectory(lib/sqlite)

add_executable(blackbook
    src/main.cxx
    src/id3.cxx
    src/song.cxx
    src/tags.cxx
    src/sqlgen.cxx

    src/arguments.cxx
    src/scope_timer.cxx
    src/stringcase.cxx

    src/cli/cli_main.cxx

    src/nativegui/gui_bootstrap.cxx
    src/nativegui/imgui_utils.cxx
    src/nativegui/nativegui.cxx
    src/nativegui/runtime_records.cxx
    src/nativegui/nfde_wrapper.cxx

    lib/glad/src/glad.c
)

# We dynamically load this in SQLite to add CRC32 support for SQL code
add_library(sqlite_crc32 SHARED
    lib/sqlite-crc32.c
)
target_link_libraries(sqlite_crc32 PUBLIC bobtail)

target_link_libraries(blackbook PUBLIC bobtail sqlite3-static glfw imgui nfd)
