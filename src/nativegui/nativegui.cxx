#include "nativegui.hxx"
#include <imgui.h>

#include <common/logging.h>
#include <common/vfile.h>

bool nativegui::load_songs(sqlite3* db) {
    return false;
}

bool nativegui::load_tags(sqlite3* db) {
    return false;
}

nativegui::nativegui(sqlite3* db) {
    bool result = true;

    result &= load_tags(db);
    result &= load_songs(db);

    initialized = result;
}

bool gui_main(void* ctx, GLFWwindow* window) {
    nativegui* gui = (nativegui*) ctx;

    ImGui::ShowDemoWindow();

    return true;
}
