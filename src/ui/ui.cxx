#include <vector>
#include <locale>
#include <codecvt>

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "ui.hxx"
#include "../hid.hxx"
#include "../hid_descriptor.hxx"
#include "imgui/imgui.h"

#define KV_PAIR(label, fmt, ...) \
	ImGui::TableNextColumn(); \
	ImGui::Text(label); \
	ImGui::TableNextColumn(); \
	ImGui::Text(fmt, __VA_ARGS__);

inline void RenderHex(const char *data, size_t dataSz);

struct {
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

    if (ImGui::Begin("Device List##device_list", &state.device_list.shown, flags)) {

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
        ImVec2 button_sz = ImVec2((wsz.x/2)-8, 16);

        if (ImGui::Button("Open All", button_sz)) {
            for (auto& [_, v] : state.shown_devices) v = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close All", button_sz)) {
            for (auto & [_, v] : state.shown_devices) v = false;
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
    if (ImGui::Begin("Device Debugger##device_debugger", &state.device_debugger.shown)) {
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
                
                if (ImGui::TreeNode("Report Descriptor")) {
                    auto dev = HID::GlobalDeviceManager.get_device(device);

                    if (ImGui::TreeNode("View")) {
                        auto desc = HID::Descriptor::parse(dev->report_descriptor.data, dev->report_descriptor.length);
                        
                        if (ImGui::TreeNode("Inputs")) {
                            
                            for ( auto input : desc.inputs ) {
                                char label[64];

                                sprintf(label, "%04x/%04x", input.usage_page, input.usage_id);
                                if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {
                                    auto def = HID::Descriptor::find_usage_definition(input.usage_page, input.usage_id);

                                    ImGui::Text("Name: %s", def.name);
                                    ImGui::Text("Offset: %04x", input.report_index);
                                    ImGui::Text("Size: %d", input.report_size);

                                    ImGui::TreePop();
                                }
                            }

                            ImGui::TreePop();
                        }

                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Save to File")) {
                        static char filePath[256];
                        ImGui::InputText("File Path", filePath, 256);
                        if (ImGui::SmallButton("Save")) {
                            FILE *file = fopen(filePath, "w");
                            fwrite(dev->report_descriptor.data, sizeof(unsigned char), dev->report_descriptor.length, file);
                            fclose(file);

                            if (ImGui::BeginPopup("hid_report_saved", ImGuiWindowFlags_Popup)) {
                                ImGui::Text("Saved Report to %s", filePath);
                                ImGui::EndPopup();
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }
        
    }
    ImGui::End();
}


inline void RenderDevice(const hid_device_info *device, bool *open) {
    static char title[256];
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    sprintf_s(title, "%ls #%d ##%p", device->product_string, device->interface_number, (void*)device);

    if (ImGui::Begin(title, open, flags)) {
        const HID::DeviceInfo *dev = HID::GlobalDeviceManager.get_device(device);
        const HID::DeviceBuffer *data = &(dev->buffers[dev->current_buffer]);

        ImGui::TextUnformatted( fmt::format("Report Date: {}", data->lru).c_str() );
        ImGui::Text("Report Length: %d", data->length);
        
        ImGui::Text("Device Location: 0x%08x", dev);
        ImGui::Text("Current Report Index: %3d", dev->current_buffer);
        ImGui::Text("Current Report Location: 0x%08x", data);
        ImGui::Spacing();

        if (data->length >= 0) {
            if (ImGui::CollapsingHeader("Raw Data")) { 
                RenderHex((const char*)data->buffer, data->length);
                ImGui::NewLine();
            }

            if (ImGui::CollapsingHeader("Input Graphs")) {
                auto w = ImGui::GetWindowSize().x - 128;
                w = w > 256 ? w : 256;
                auto desc = HID::Descriptor::parse(dev->report_descriptor.data, dev->report_descriptor.length);

                for (auto input : desc.inputs) {
                    float *series = HID::GlobalDeviceManager.get_input_series(device, &input);

                    auto def = HID::Descriptor::find_usage_definition(input.usage_page, input.usage_id);
                    
                    if (input.report_size == 1) {
                        ImGui::PlotHistogram(
                            fmt::format("{}\n{:#04x}", def.name, input.usage_id).c_str(), // Label
                            series,                                                       // Series Data,
                            HID::NUM_BUFFERS,                                             // Series Length
                            dev->current_buffer,                                          // Series Offset,
                            fmt::format("{:05.0f}", series[dev->current_buffer]).c_str(), // Overlay text
                            0,                                                            // input.min_value,                                              // Minimum Value
                            1,
                            ImVec2(w, 48.0f)                                              // Graph Size
                        );
                    } else {

                        ImGui::PlotLines(
                            fmt::format("{}\n{:#04x}", def.name, input.usage_id).c_str(), // Label
                            series,                                                       // Series Data,
                            HID::NUM_BUFFERS,                                             // Series Length
                            dev->current_buffer,                                          // Series Offset,
                            fmt::format("{:05.0f}", series[dev->current_buffer]).c_str(), // Overlay text
                            0,                                                            // input.min_value,                                              // Minimum Value
                            1 << (input.report_size),                                       // input.max_value,                                              // Maximum Value
                            ImVec2(w, 48.0f)                                              // Graph Size
                        );
                    }

                    free(series);
                }
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.3f, 0.3f, 1.0f), "Error Reading from Device");
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