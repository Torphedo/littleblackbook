cmake_minimum_required(VERSION 3.13)
project(blackbook
    VERSION 1.0.0
    LANGUAGES CXX C
)

# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
# Enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# We're not using ICU for now because it creates a runtime dependency that's
# annoying to resolve when set up like this.
# find_package(ICU COMPONENTS i18n io uc)
# message("Was ICU found? ${ICU_FOUND} (v${ICU_VERSION})")
# message("ICU libraries: ${ICU_LIBRARIES}")
# message("ICU include dir is: ${ICU_INCLUDE_DIRS}")

include_directories(
    src
    lib
    lib/bobtail

    lib/imgui
    lib/glad/include
    lib/sqlite
    lib/glfw/include
    lib/raudio/src
    lib/nfde/src/include

    ${ICU_INCLUDE_DIRS}
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
add_library(stb_image STATIC
    lib/stb_image.c
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
set(ENABLE_DYNAMIC_EXTENSIONS ON)
set(BUILD_SHELL OFF)
set(ENABLE_FTS4 OFF) # We only use FTS5
set(ENABLE_RTREE OFF)
set(OMIT_JSON OFF)
set(OMIT_AUTOINIT OFF)
add_subdirectory(lib/sqlite)

add_executable(blackbook
    src/main.cxx
    src/id3.cxx
    src/import.cxx
    src/tags.cxx
    src/sqlgen.cxx
    src/expression.cxx
    src/blackbook_core.cxx

    src/nativegui/gui_bootstrap.cxx
    src/nativegui/nativegui.cxx
    src/nativegui/thumbnails.cxx
    src/nativegui/nfde_wrapper.cxx
    src/nativegui/imgui_utils.cxx

    src/cli/cli_main.cxx

    # Generic sources
    src/arguments.cxx
    src/scope_timer.cxx
    src/text_i8n.cxx

    src/sqlite-crc32.c

    lib/glad/src/glad.c # Can't be bothered to make this its own target
    src/autocomplete.cxx
)

target_link_libraries(blackbook PUBLIC
    bobtail
    sqlite3-static glfw imgui nfd raudio stb_image
)

if (WIN32)
    # SQLite needs this to generate UUIDs
    target_link_libraries(blackbook PUBLIC Rpcrt4)
endif()
