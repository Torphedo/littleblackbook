# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("blackbook_android")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(BLACKBOOK_ROOT "../../../../..")
set(IMGUI_DIR "${BLACKBOOK_ROOT}/lib/imgui")

# Add ImGui as a library
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

add_subdirectory("${BLACKBOOK_ROOT}/src/core" blackbook_core)

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(blackbook SHARED
    main.cpp
    AndroidOut.cpp
    Utility.cpp
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Forces the linker to keep the JNI entry point for GameActivity
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# Configure libraries CMake uses to link your target library.
target_link_libraries(blackbook
    blackbook_core

    # The game activity
    game-activity::game-activity_static

    # EGL and other dependent libraries required for drawing
    # and interacting with Android system
    EGL
    GLESv3
    jnigraphics
    android
    log
)
