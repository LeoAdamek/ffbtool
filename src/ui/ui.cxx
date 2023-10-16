#include <vector>
#include <locale>
#include <codecvt>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "ui.hxx"
#include "../hid.hxx"
#include "imgui/imgui.h"

#define KV_PAIR(label, fmt, ...) \
	ImGui::TableNextColumn(); \
	ImGui::Text(label); \
	ImGui::TableNextColumn(); \
	ImGui::Text(fmt, __VA_ARGS__);

inline void RenderHex(const char *data, size_t dataSz);

static struct {
    struct {
        bool imgui_demo;
        bool imgui_debug;
    } main_menu;

    struct {
       bool shown = true ;
    } device_list;

    struct {
        bool shown = false ;
    } device_debugger;

    std::map<char*, bool> shown_devices;
} state;

std::string w2s(const std::wstring& in);

// Window rendering functions
inline void RenderDeviceList();
inline void RenderDeviceDebugger();
inline void RenderDevice(const hid_device_info *device, bool *open);

void UI::Render() {

    if (state.shown_devices.empty()) {
        for (auto device = HID::GlobalDeviceManager.get_devices(); device; device = device->next) {
            state.shown_devices.emplace(device->path, false);
        }
    }

    RenderDeviceList();
    if (state.device_debugger.shown) RenderDeviceDebugger();

    if (state.main_menu.imgui_demo) ImGui::ShowDemoWindow(&state.main_menu.imgui_demo);
    if (state.main_menu.imgui_debug) ImGui::ShowMetricsWindow(&state.main_menu.imgui_debug);
}

inline void RenderDeviceList() {
    ImGuiViewport *vp = ImGui::GetMainViewport();
    static ImGuiWindowFlags flags = NULL;

    if (ImGui::Begin("Device List", &state.device_list.shown, flags)) {

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Devices")) {
                ImGui::MenuItem("Device List", "", &state.device_list.shown);
                ImGui::MenuItem("Device Debugger", "", &state.device_debugger.shown);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("ImGui Debug", "", &state.main_menu.imgui_debug);
                ImGui::MenuItem("ImGui Demo", "", &state.main_menu.imgui_demo);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImVec2 wsz = ImGui::GetWindowSize();

        if (ImGui::Button("Open All", ImVec2(wsz.y, 16))) {
            for (auto s : state.shown_devices) {
                s.second = true;
            }
        }

        for (auto device = HID::GlobalDeviceManager.get_devices(); device; device = device->next) {
            char label[512];
            sprintf_s(label, "%ls #%d  ##%s", device->product_string, device->interface_number, device->path);

            auto shown = state.shown_devices.find(device->path);
            ImGui::Selectable(label, &shown->second);
            if (shown->second) RenderDevice(device, &shown->second);
        }
    }
    ImGui::End();
}

inline void RenderDeviceDebugger() {
    if (ImGui::Begin("Device Debugger", &state.device_debugger.shown)) {
        const hid_device_info *devices = HID::GlobalDeviceManager.get_devices();

        for (auto device = devices; device; device = device->next) {
            char device_name[512];
            sprintf_s(device_name, "%ls ##%s", device->product_string, device->path);

            if (ImGui::TreeNode(device_name)) {
                
                if (ImGui::TreeNode("Device Info")) {
                    if (ImGui::BeginTable("device_info", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable )) {
                        KV_PAIR("Vendor ID", "0x%04x", device->vendor_id);
                        KV_PAIR("Product ID", "0x%04x", device->product_id);
                        KV_PAIR("Manufacturer String", "%ls", device->manufacturer_string);
                        KV_PAIR("Product String", "%ls", device->product_string);
                        KV_PAIR("Serial Number", "%ls", device->serial_number);
                        KV_PAIR("Path", "%s", device->path);
                        KV_PAIR("Usage Page", "0x%04x", device->usage_page);
                        KV_PAIR("Usage", "0x%04x", device->usage);
                        KV_PAIR("Interface #", "0x%04x", device->interface_number);
                        KV_PAIR("Release #", "0x%04x", device->release_number)
                        ImGui::EndTable();   
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            };
        }
        
    }
    ImGui::End();
}


inline void RenderDevice(const hid_device_info *device, bool *open) {
    char title[512];
    sprintf_s(title, "%ls %ls #%d ##%s", device->manufacturer_string, device->product_string, device->interface_number, device->path);
    if (ImGui::Begin(title, open)) {
        HID::GlobalDeviceManager.get_update_rate(device);
        const HID::DeviceBuffer *data = HID::GlobalDeviceManager.get_latest_report(device);
        ImGui::TextUnformatted( fmt::format("Report Date: {}", data->lru).c_str() );
        ImGui::Text("Report Length: %d", data->length);
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Raw Data")) RenderHex((const char*)data->buffer, data->length);

        ImGui::NewLine();

        if (ImGui::CollapsingHeader("Value Series")) {
            for (auto i = 0; i < data->length; i++) {
                int value = (int)data->buffer[i] | (int)(data->buffer[++i]) << 8;

                ImGui::SliderInt(fmt::format("{:04x}", i).c_str(), &value, 0, 0xFFFF, "%05d", ImGuiSliderFlags_NoInput);
            }
        }
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