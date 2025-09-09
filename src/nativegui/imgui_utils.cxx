#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.hxx"
#include <misc/cpp/imgui_stdlib.h>
#include "blackbook_core.hxx"
#include "scope_timer.hxx"
#include "common/logging.h"

namespace ImGui {
    void Indent(u32 tab_num) {
        for (u32 i = 0; i < tab_num; i++) {
            ImGui::Text("\t"); ImGui::SameLine();
        }
    }

    static int autocomplete_update_selection(ImGuiInputTextCallbackData* data) {
        auto tac = (tag_autocomplete*) data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            if (tac->cur_idx != 0) {
                // User edited a different buffer, it should become the new main buffer
                tac->need_apply = true;
            }
            tac->need_refresh = true; // Need to refresh results
            return 0;
        }

        if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory) {
            return 0;
        }

        // 1 if down, -1 if up, 0 if both.
        const s8 diff = (data->EventKey == ImGuiKey_DownArrow) - (data->EventKey == ImGuiKey_UpArrow);
        const bool prefix_minus = data->Buf[0] == '-';

        tac->update_selection(diff);
        tac->need_refocus = true;

        return 0;
    }

    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac, blackbook_core& core) {
        bool result = false;
        // ImGui will try to save pointers internally, under the assumption that
        // inputs with the same label are the same std::string* every time.
        //
        // If we change that pointer between calls, it can cause autocomplete results
        // to be overwritten with the current user input. We work around this by
        // giving a unique label to each result.
        // P.S. This might be a bug on ImGui's side, I'm not sure. - torph
        //
        // TODO: Handle dynamic strings manually in a custom callback to avoid extra custom labels.
        const std::string real_label = label + std::to_string(tac.cur_idx);

        // We need a callback to make this work. History == up/down keys
        flags |= ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
        if (ImGui::InputTextWithHint(real_label.c_str(), hint, tac.user_str, flags, autocomplete_update_selection, &tac)) {
            result = true;
            tac.need_refocus = true;
            tac.need_apply = true;
        }

        result |= tac.need_apply;

        // This is done via flag since it can invalidate pointers, which is a problem
        // in callbacks.
        if (tac.need_apply) {
            tac.apply_selection();
            tac.need_apply = false;
        }

        // This is done via flag so we have the db ptr and access to timer output
        if (tac.need_refresh) {
            const scope_timer main_timer(core.timer_map, "tag_autocomplete");
            tac.update_results(core.db);
            tac.need_refresh = false;
        }

        // Draw results
        for (u32 i = 0; i < tac.candidates.size(); i++) {
            const std::string& candidate = tac.candidates[i];
            const ImVec4 green = ImVec4(0, 255, 0, 255);
            const ImVec4 white = ImVec4(255, 255, 255, 255);
            const ImVec4 color = ((i + 1) == tac.cur_idx) ? green : white;

            ImGui::TextColored(color, "%s", candidate.c_str());
        }

        if (!tac.candidates.empty()) {
            ImGui::Separator();
        }

        return result;
    }

    void TagOpDropDown(tag_op& op) {
        char idbuf[64] = {0};
        snprintf(idbuf, sizeof(idbuf) - 1, "opCombo%p", &op);

        ImGui::PushID(idbuf);
        int flags = ImGuiComboFlags_WidthFitPreview;
        if (ImGui::BeginCombo("", tag_op_strs[(u8)op], flags)) {
            for (u8 i = (u8)tag_op::AND; i < (u8)tag_op::NOT; i++) {
                const std::string id = "opComboOption" + std::to_string(i);
                ImGui::PushID(id.c_str());
                if (ImGui::Selectable(tag_op_strs[i], i == (u8)op)) {
                    op = (tag_op)i;
                }
                ImGui::PopID();
            }

            ImGui::EndCombo();
        }
        ImGui::PopID();
    }

    bool EditExpressionValue(tag_expression::value& val, tag_autocomplete& tac, blackbook_core& core, u32 indent) {
        if (VAL_IS_EXPR(val)) {
            return EditExpression(*std::get<tag_expression*>(val), tac, core, indent + 1);
        } else if (VAL_IS_IMM(val)) {
            Indent(indent);
            auto& str = std::get<std::string>(val);
            const char* hint = "Input a tag (expression)";
            const std::string label = "##" + std::to_string((uintptr_t)&str);

            // Since the user can only focus & type in 1 box at a time, we only
            // have 1 TAC. A TAC input looks/acts just like a normal one when
            // not focused, so this gives the illusion that they are all TAC
            // inputs with their own state.
            bool result = false;
            if (&str == tac.user_str) {
                if (tac.need_refocus) {
                    ImGui::SetKeyboardFocusHere();
                    tac.need_refocus = false;
                }
                result = InputTagAutocompleted(label.c_str(), hint, 0, tac, core);

                if (result) {
                    std::queue<substr_t> tokens = shatter_str(tac.user_str->c_str(), tac.user_str->length());
                    if (tokens.size() > 1) {
                        // This is an actual expression and not a tag, run it through the parser
                        tag_expression* expr = new tag_expression(tokens);
                        val = expr;
                    }
                }
            } else {
                InputTextWithHint(label.c_str(), hint, &str);
                if (ImGui::IsItemFocused() || tac.user_str == nullptr) {
                    tac.user_str = &str;
                    tac.update_results(core.db);
                    tac.need_refocus = true;
                }
            }

            if (result) {
                if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
                    // add a new tag input by turning immediate into an expression
                    const std::string tag = str;
                    val = tag_expression(tag.c_str());
                    // TODO: Make the child value inherit the parent's operator
                } else {
                    // This causes focus to move to the next box
                    tac.user_str = nullptr;
                }
            }

            return result;
        } else {
            LOG_MSG(error, "Expression node value is in an invalid state!\n");
            return false;
        }
    }

    bool EditExpression(tag_expression& expr, tag_autocomplete& tac, blackbook_core& core, u32 indent) {
        bool result = false;

        Indent(indent);
        TagOpDropDown(expr.op);
        result = EditExpressionValue(expr.lhs, tac, core, indent);
        result |= EditExpressionValue(expr.rhs, tac, core, indent);

        return result;
    }
} // namespace ImGui
