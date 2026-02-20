#pragma once
#include <imgui.h>
#include "expression.hxx"
#include <blackbook_core.hxx>

namespace ImGui {
    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac, blackbook_core& core);

    void TagOpDropDown(tag_op& op);
    bool EditExpression(tag_expression& expr, tag_autocomplete& tac, blackbook_core& core, u32 indent = 0);

    void OffsetCursorY(float dist);

    static float CharWidth(u32 num_chars = 1) {
        return ImGui::CalcTextSize("x").x * float(num_chars);
    }

    static float CharHeight() {
        return ImGui::CalcTextSize("I").y;
    }
} // namespace ImGui
