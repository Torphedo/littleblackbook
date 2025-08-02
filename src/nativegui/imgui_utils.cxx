#include "imgui_utils.hxx"
#include <misc/cpp/imgui_stdlib.h>
#include "blackbook_core.hxx"
#include "scope_timer.hxx"

namespace ImGui {
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
        const auto old_idx = tac.cur_idx;
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
        flags |= ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit;
        if (ImGui::InputTextWithHint(real_label.c_str(), hint, &tac.current(), flags, autocomplete_update_selection, &tac)) {
            result = true;
            tac.need_refocus = true;
        }

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
        for (const std::string& candidate : tac.candidates) {
            ImGui::Text("%s", candidate.c_str());
        }
        ImGui::Separator();

        return result;
    }
} // namespace ImGui
