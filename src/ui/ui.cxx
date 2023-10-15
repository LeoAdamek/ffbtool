#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <locale>
#include <set>
#include <codecvt>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "ui.hxx"
#include "../hid.hxx"
#include "imgui/imgui.h"

void RenderMainMenu();
inline void RenderHex(const char *data, size_t dataSz);

static struct {
    std::vector<HID::Device> devices;
    std::set<uint32_t> selected;
    char **device_labels;

    struct {
        bool debug = false;
        bool imgui_demo = false;
        bool imgui_debug = false;
    } main_menu;

    struct {
        bool shown;
    } device_list;

    struct {
        uint32_t idx;
        uint16_t usage_page;
        uint16_t usage;
        uint8_t data[256];
        uint8_t dptr;
    } samples[32];

} state;

std::string w2s(const std::wstring& in);

void RenderDeviceWindow(uint32_t idx);

void UI::Render() {

    ImGuiViewport *vp = ImGui::GetMainViewport();
    //ImGui::SetNextWindowPos(ImVec2(0, 17));
    //ImGui::SetNextWindowSize(ImVec2(256, vp->Size.y - 17));

    if (state.devices.empty()) {
        state.devices = HID::ListDevices();
        auto devicesSz = state.devices.size();

        state.device_labels = (char**)calloc(devicesSz, sizeof(char*));

        for (auto i = 0; i < devicesSz; i++) {
            auto device = state.devices.at(i);
            std::string product_name = w2s(device.product_name);

            state.device_labels[i] = (char*)malloc(256);
            sprintf_s(state.device_labels[i], 256, "(%04x:%04x) %s", device.vendor_id, device.product_id, product_name.c_str());
        }
    }

    static ImGuiWindowFlags flags = NULL;

    if (ImGui::Begin("Device List", &state.device_list.shown, flags)) {

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("ImGui Debug", "", &state.main_menu.imgui_debug);
                ImGui::MenuItem("ImGui Demo", "", &state.main_menu.imgui_demo);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        for (auto i = 0; i < state.devices.size(); i++) {
            if (ImGui::Selectable(state.device_labels[i], state.selected.contains(i))) {
                state.selected.emplace(i);
            }
        }

        for (auto idx : state.selected ) {
            RenderDeviceWindow(idx);
        }
    }

    if (state.main_menu.imgui_demo) ImGui::ShowDemoWindow(&state.main_menu.imgui_demo);
    if (state.main_menu.imgui_debug) ImGui::ShowMetricsWindow(&state.main_menu.imgui_debug);

    ImGui::End();
}


void RenderDeviceWindow(uint32_t idx) {
    const char* label = state.device_labels[idx];
    HID::Device device = state.devices.at(idx);

    static int report_id = 0;
    static int bufferSz = 256;

    //ImGui::SetNextWindowSize(ImVec2(480, 256));

    if (ImGui::Begin(label)) {
        char *buffer = (char*)malloc(bufferSz);
        memset(buffer, 0, bufferSz);

        ImGui::InputInt("Report ID", &report_id, 1);
        ImGui::InputInt("Report Length", &bufferSz, 1);
        buffer[0] = report_id;
        size_t reportSz = device.Read(buffer, bufferSz);
        
        ImGui::Separator();

        ImGui::Text("Report Length: %zu", reportSz);

        RenderHex(buffer, reportSz);

        auto samples = &state.samples[idx];
        uint8_t *data = samples->data;

        data[samples->dptr++] = buffer[0x01];
        data[samples->dptr++] = buffer[0x02];
        
        ImGui::NewLine();
        ImGui::SeparatorText("Sample Data");
        RenderHex((char*)data, 256);

        free(buffer);
    }

    ImGui::End();
}


std::string w2s(const std::wstring& in) {
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    return converter.to_bytes(in);
}

inline void RenderHex(const char *data, size_t dataSz) {
    for (auto i = 0; i < dataSz; i++) {
        std::string text = fmt::format("{:02X}", data[i]);
        ImGui::Text(text.c_str());

        if (i == 0 || i % 16 != 0) {
            ImGui::SameLine();
        } 
    }
}