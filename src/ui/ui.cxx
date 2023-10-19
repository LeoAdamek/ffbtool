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

#define INPUT_ID(VD_ID, PR_ID, RP_ID, RP_X) \
    (((uint64_t) VD_ID << 48) | ((uint64_t)PR_ID << 32) | ((uint16_t)RP_ID << 16) | ((uint16_t)RP_X))

inline void RenderHex(const char *data, size_t dataSz);

const uint8_t label_length = 0xFF;

struct {

    char save_path[256];

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

    std::map<uint64_t, std::pair<uint8_t,char*>> input_names;
} state;

std::string w2s(const std::wstring& in);

// Window rendering functions
inline void RenderDeviceList();
inline void RenderDeviceDebugger();
inline void RenderDevice(const hid_device_info *device, bool *open);

void UI::Setup() {
    for (auto device = HID::GlobalDeviceManager.get_devices(); device; device = device->next) {
        state.shown_devices.emplace(device->path, false);
    }

    // Load the labels
    FILE *labels = fopen("labels.dat", "r");

    uint64_t input_id;
    uint8_t input_length;
    char *input_name;

    bool eof = false;

    if (labels) {
        while(true) {
            if (fread(&input_id, 8, 1, labels) != 1) break;
            if (fread(&input_length, 1, 1, labels) != 1) break;
            input_name = (char*)malloc(label_length);
            memset(input_name, 0, label_length);
            if (fread(input_name, sizeof(char), input_length, labels) != input_length) break;

            state.input_names.emplace(input_id, std::make_pair(label_length, input_name));
        }
    }
}

void UI::Render() {
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

void RenderDeviceDebugger(const hid_device_info *device) {
    ImGui::PushID(device);
    
    auto dev = HID::GlobalDeviceManager.get_device(device);

    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();

    bool open = ImGui::TreeNode(device, "Device: %x", (void*)device);

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%ls", device->product_string);

    if (open) {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
            | ImGuiTreeNodeFlags_NoTreePushOnOpen
            | ImGuiTreeNodeFlags_Bullet;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TreeNodeEx("manufacturer", flags, "Manufacturer String");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Text("%ls", device->manufacturer_string);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TreeNodeEx("product_name", flags, "Product String");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Text("%ls", device->product_string);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TreeNodeEx("vendor_id", flags, "Vendor ID");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Text("0x%04x", device->vendor_id);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TreeNodeEx("vendor_id", flags, "Product ID");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Text("0x%04x", device->product_id);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();

        if (ImGui::TreeNode("Report Descriptor")) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();

            auto descriptor = HID::Descriptor::parse(dev->report_descriptor.data, dev->report_descriptor.length);

            if (ImGui::TreeNode("Inputs")) {

                for (auto& input : descriptor.inputs) {
                    uint64_t input_id = INPUT_ID(device->vendor_id, device->product_id, input.report_id, input.report_index);
                    auto def = HID::Descriptor::find_usage_definition(input.usage_page, input.usage_id);
                    auto it = state.input_names.find(input_id);

                    char *label = def.name;

                    if (it != state.input_names.end()) {
                        label = it->second.second;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    bool open = ImGui::TreeNode((void*)input_id, "Input");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s (%04x,%04x) \"%s\"", def.name, input.usage_page, input.usage_id, label);

                    if (open) {

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TreeNodeEx("report_id", flags, "Report ID");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::Text("%d", input.report_id);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TreeNodeEx("report_index", flags, "Report Offset (Bits)");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::Text("%d", input.report_index);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TreeNodeEx("report_size", flags, "Report Size (Bits)");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::Text("%d", input.report_size);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TreeNodeEx("usage_page", flags, "Usage Page");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::Text("0x%04x", input.usage_page);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TreeNodeEx("usage_page", flags, "Usage ID");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::Text("%s (0x%04x)", def.name, input.usage_id);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();

                        bool showFlags = ImGui::TreeNode("Flags");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::Text("%d", input.properties.size());

                        if (showFlags) {
                            for (auto flag : input.properties) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(1);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%d", flag);
                            }

                            ImGui::TreePop();
                        }


                        if (it == state.input_names.end()) {
                            uint8_t buffer_sz = label_length;
                            char *buffer = (char*)malloc(buffer_sz);
                            memset(buffer, 0, buffer_sz);
                            state.input_names.emplace(input_id, std::make_pair(buffer_sz, buffer));
                        }

                        auto label = state.input_names[input_id];

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TreeNodeEx("label", flags, "Custom Label");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        ImGui::InputText("x", label.second, label.first);

                        ImGui::TreePop(); 
                    }
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Save Report")) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TreeNodeEx("report_file_path", flags, "File Path");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText("", state.save_path, 256);

                ImGui::TreePop();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                ImGui::AlignTextToFramePadding();
                ImGui::SetNextItemWidth(-FLT_MIN);

                if (ImGui::Button("Save") ) {
                    FILE *file = fopen(state.save_path, "w");
                    fwrite(dev->report_descriptor.data, sizeof(unsigned char), dev->report_descriptor.length, file);
                    fclose(file);
                }
            }

            ImGui::TreePop();   
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

static void RenderDeviceDebugger() {
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Device Debugger##device_debugger", &state.device_debugger.shown)) {

        ImVec2 button_sz = ImGui::GetWindowSize();
        button_sz.y = 16;
        button_sz.x -= 16;
        
        if (ImGui::Button("Save Labels", button_sz)) {
            FILE *file = fopen("labels.dat", "w");
            for (auto [id, label] : state.input_names) {
                uint8_t length = strnlen(label.second, label.first);

                if (length > 0) {
                    fwrite(&id, sizeof(uint64_t), 1, file);
                    fwrite(&length, sizeof(uint8_t), 1, file);
                    fwrite(label.second, sizeof(char), length, file);
                }
            }

            fclose(file);
        }

        const hid_device_info *devices = HID::GlobalDeviceManager.get_devices();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));

        ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("##device_debugger_table", 2, flags)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Property");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            for(auto device = devices; device; device = device->next) {
                RenderDeviceDebugger(device);
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();

    }
    ImGui::End();
}


inline void RenderDevice(const hid_device_info *device, bool *open) {
    static char title[256];
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    sprintf_s(title, "%ls #%d ##%p", device->product_string, device->interface_number, (void*)device);

    uint64_t device_id = (device->vendor_id << 16 | device->product_id);
    device_id <<= 32;

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
                auto w = ImGui::GetWindowSize().x - 192;
                w = w > 256 ? w : 256;
                auto desc = HID::Descriptor::parse(dev->report_descriptor.data, dev->report_descriptor.length);

                for (auto input : desc.inputs) {
                    uint64_t input_id = device_id | (input.report_id << 16) | (uint16_t)input.report_index;

                    float *series = HID::GlobalDeviceManager.get_input_series(device, &input);

                    auto def = HID::Descriptor::find_usage_definition(input.usage_page, input.usage_id);
                    
                    if (input.report_size == 1) {
                        ImGui::PlotHistogram(
                            fmt::format("{:#16x}", input_id).c_str(), // Label
                            series,                                                       // Series Data,
                            HID::NUM_BUFFERS,                                             // Series Length
                            dev->current_buffer,                                          // Series Offset,
                            "",
                            0,                                                            // Minimum Value
                            1,
                            ImVec2(w, 16.0f)                                              // Graph Size
                        );
                    } else {
                        ImGui::PlotLines(
                            fmt::format("{}\n{:#04x}", def.name, input.usage_id).c_str(), // Label
                            series,                                                       // Series Data,
                            HID::NUM_BUFFERS,                                             // Series Length
                            dev->current_buffer,                                          // Series Offset,
                            fmt::format("{:05.0f}", series[dev->current_buffer]).c_str(), // Overlay text
                            0,                                                            // input.min_value,                                              // Minimum Value
                            1 << (input.report_size),                                     // input.max_value,                                              // Maximum Value
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