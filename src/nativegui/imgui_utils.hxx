#pragma once
#include <imgui.h>
#include "expression.hxx"

namespace ImGui {
    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac, blackbook_core& core);
} // namespace ImGui
