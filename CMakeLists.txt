cmake_minimum_required(VERSION 3.13)
project(blackbook
    VERSION 1.0.0
    LANGUAGES CXX C
)

# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
# Enable LTO
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

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
    src/core
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

add_subdirectory(lib/nfde)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(lib/glfw)

set(BOBTAIL_OPENGL ON)
add_subdirectory(src/core)

add_executable(blackbook
    src/main.cxx

    src/nativegui/gui_bootstrap.cxx
    src/nativegui/nativegui.cxx
    src/nativegui/thumbnails.cxx
    src/nativegui/nfde_wrapper.cxx
    src/nativegui/imgui_utils.cxx

    src/cli/cli_main.cxx

    lib/glad/src/glad.c # Can't be bothered to make this its own target
)

target_link_libraries(blackbook PUBLIC
    blackbook_core glfw imgui nfd 
)
