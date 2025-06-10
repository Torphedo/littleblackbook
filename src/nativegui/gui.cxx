#include <cstdio>
#include <imgui.h>

#include <common/logging.h>
#include <common/vfile.h>

#include "gui_loop.hxx"

bool gui_main(void* ctx, GLFWwindow* window) {
    ImGui::ShowDemoWindow();

    return true;
}
