#pragma once
#include <imgui.h>
#include "expression.hxx"
#include <blackbook_core.hxx>

namespace ImGui {
    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac, blackbook_core& core);

    void TagOpDropDown(tag_op& op);
    bool EditExpression(tag_expression& expr, tag_autocomplete& tac, blackbook_core& core, u32 indent = 0);
} // namespace ImGui
