#include "../ui/imgui/imgui.h"
#include "pov_hat.hxx"

namespace Widgets {
    void POVHat(const void *id, const uint8_t value, float radius) {
        
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        ImVec2 center = ImVec2( pos.x - radius/2, pos.y - radius/2 );

        draw_list->AddCircle( center, radius, IM_COL32(255, 0, 0, 255) );
    }
}