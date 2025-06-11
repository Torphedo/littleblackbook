#pragma once
#include <GLFW/glfw3.h>

typedef bool (*gui_callback)(void* ctx, GLFWwindow* window);

/// @brief Underlying GUI loop for the program
///
/// Handles the main loop and setup/teardown of ImGui & GLFW
/// @return Returns false if unable to create the UI
bool gui_loop(gui_callback callback, void* ctx);
