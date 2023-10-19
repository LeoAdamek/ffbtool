#include <array>
#include <bitset>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "hid_descriptor.hxx"

#if (_DEBUG)
    //#define LOG(FMT, ...) fmt::println(FMT, __VA_ARGS__)
    #define LOG(FMT, ...)
#else
    #define LOG(FMT, ...) 
#endif

namespace HID {
    namespace Descriptor {

        using GlobalParams = std::array<int32_t, 10>;
        using LocalParams = std::map<LocalItemTag, std::vector<int32_t>>;

        uint32_t index = 0;

        int32_t parse_hid_report_get_item_data(const uint8_t *buffer, uint8_t item_sz) {
            if (item_sz == 1) return *buffer;             // Return 1 byte
            if (item_sz == 2) return *((int16_t*)buffer); // Return 2 bytes
            if (item_sz == 4) return *((int32_t*)buffer); // Return 4 bytes
            return 0;
        }

        uint8_t item_size(uint8_t value);
        UsagePage usage_page(uint8_t value);
        ReportItemType report_item_type(uint8_t value);

        void global_item(uint8_t tag, uint8_t data_sz, int32_t data, std::vector<GlobalParams> *state);
        void main_item  (
            Descriptor *d, 
            uint8_t tag, 
            uint8_t data_sz, 
            int32_t data, 
            GlobalParams params, 
            LocalParams *local, 
            uint8_t *collection_depth
        );

        void local_item (uint8_t tag, int32_t data, LocalParams *state);
        void input      (Descriptor *d, uint8_t data_sz, int32_t data, GlobalParams params, LocalParams *local);

        Descriptor parse(const unsigned char *buffer, size_t buffer_sz) {
            size_t idx = 0;
            index = 0;

            // Global state values which can be changed by a GLOBAL command
            // These can be stacked... because of course..
            std::vector<GlobalParams> state;
            state.reserve(2);
            state.push_back({});
            
            uint8_t collection_depth = 0;
            LocalParams local_state = {};
            Descriptor descriptor = {};

            while(idx < buffer_sz) {
                uint8_t mark = buffer[idx];
                uint8_t item_sz  = item_size(buffer[idx] & SIZE_MASK);

                int32_t data = parse_hid_report_get_item_data(&buffer[idx+1], item_sz);

                ReportItemType item_type = (ReportItemType)(mark & TYPE_MASK);
                uint8_t tag = mark >> 4;

                switch (item_type) {
                    case ReportItemType::MAIN_ITEM:
                        main_item(&descriptor, tag, item_sz, data, state.back(), &local_state, &collection_depth);

                        // Local state does not carry over to the next main item out of a collection
                        if (collection_depth == 0 || tag == MainItemTag::END_COLLECTION) local_state = {};
                        break;
                    case ReportItemType::GLOBAL_ITEM:
                        global_item(tag, item_sz, data, &state);
                        break;
                    case ReportItemType::LOCAL_ITEM:
                        local_item(tag, data, &local_state);
                    default:
                        break;
                }

                idx += item_sz + 1;
            }

            return descriptor;
        }

        uint8_t item_size(uint8_t item) {
            uint8_t maskedSize = item & SIZE_MASK;

            return (maskedSize == 3 ? 4 : maskedSize);
        }

        UsagePage usage_page(uint8_t value) {
            return (UsagePage)(0x01 << 2 | value & SIZE_MASK);
        }

        inline void global_item(uint8_t tag, uint8_t data_sz, int32_t data, std::vector<GlobalParams> *state) {
            assert(tag <= GlobalItemTag::POP);
            auto p = state->back();

            switch(tag) {
                case GlobalItemTag::PUSH:
                    state->push_back(p);
                    break;
                case GlobalItemTag::POP:
                    if (state->size() > 1) state->pop_back();
                    break;
                default:
                    state->pop_back();
                    p[tag] = data;
                    state->push_back(p);
                    break;
            }
        }

        void main_item(Descriptor *d, uint8_t tag, uint8_t data_sz, int32_t data, GlobalParams params, LocalParams *local, uint8_t *collection_depth) {
            assert(tag <= MainItemTag::END_COLLECTION);

            switch (tag) {
                case MainItemTag::INPUT:
                    input(d, data_sz, data, params, local);
                case MainItemTag::OUTPUT:
                case MainItemTag::FEATURE:
                case MainItemTag::COLLECTION:
                    collection_depth++;
                    break;
                case MainItemTag::END_COLLECTION: 
                    collection_depth--;
                    break;
            }
        }

        void local_item(uint8_t tag, int32_t data, LocalParams *state) {
            LocalItemTag t = (LocalItemTag) tag;
            if ( state->contains(t) ) {
                state->at(t).push_back(data);
                std::rotate(state->at(t).rbegin(), state->at(t).rbegin() + 1, state->at(t).rend());
            } else {
                std::vector<int32_t> value = { data };
                state->emplace(t, value);
            }
        }

        void input(Descriptor *d, uint8_t data_sz, int32_t data, GlobalParams params, LocalParams *local) {
            int32_t report_count = params[GlobalItemTag::REPORT_COUNT],
                    report_id = params[GlobalItemTag::REPORT_ID],
                    report_size = params[GlobalItemTag::REPORT_SIZE],
                    logical_min = params[GlobalItemTag::LOGICAL_MINIMUM],
                    logical_max = params[GlobalItemTag::LOGICAL_MAXIMUM];

            auto usage_min_it = local->find(LocalItemTag::UsageMin);
            auto usage_max_it = local->find(LocalItemTag::UsageMax);

            uint16_t usage_id = 0,
                     usage_min = 0,
                     usage_max = 0;

            if (usage_min_it != local->end() && usage_max_it != local->end()) {
                if (!usage_min_it->second.empty() && !usage_max_it->second.empty()) {
                    usage_max = usage_id = (uint16_t) usage_max_it->second.back();
                    usage_min = (uint16_t) usage_min_it->second.back();
                    usage_max_it->second.pop_back();
                    usage_min_it->second.pop_back();
                }
            }

            for (auto i = 0; i < report_count; i++) {
                if (usage_max && usage_min) {
                    usage_id--;
                } else {
                    auto usage = local->find(LocalItemTag::Usage);

                    if (usage != local->end()) {
                        if (!usage->second.empty()) {
                            usage_id = usage->second.back();
                            usage->second.pop_back();
                        }
                    }
                }

                Input v = {
                    .usage_page = (UsagePage)params[GlobalItemTag::USAGE_PAGE],
                    .report_id = (uint16_t)report_id,
                    .usage_id = usage_id,
                    .report_size = (uint8_t)report_size,
                    .report_index = index,
                    .min_value = logical_min,
                    .max_value = logical_max,
                };

                index += report_size;

                for (auto b = 0; b <= 8; b++) {
                    if (data & (1<<b)) {
                        v.properties.push_back(InputProperty(b));
                    }
                }

                UsageDef def = find_usage_definition(v.usage_page, v.usage_id);

                LOG (
                    "Input<Page: {:#04x}, ReportID: {}, UsageID: {:#04x}, Size: {}, Index: {:#04x}>: {}",
                    (uint16_t)v.usage_page,
                    v.report_id,
                    v.usage_id,
                    v.report_size,
                    v.report_index,
                    def.name
                );

                d->inputs.push_back(v);
            }
        }

        UsageDef find_usage_definition(uint16_t usage_page, uint16_t usage_type) {
            if (usage_page >= 0xFF00) {
                return { (UsagePage)usage_page, 0x00, 0xFFFF, NULL, "Vendor-Defined" };
            }

            if (usage_page >= 0xF1D1 || (usage_page >= 0x93 && usage_page < 0xF1D0)) {
                return { (UsagePage)usage_page, 0x00, 0xFFFF, NULL, "RESERVED" };
            }

            for (size_t i = 0; i < USAGES_LENGTH; i++) {
                const UsageDef def = usage_definitions[i];

                if (usage_page == def.page && usage_type >= def.min && usage_type <= def.max) {
                    return def;
                }
            }

            return { UNDEFINED, 0x00, 0xFFFF, NULL, "UNDEFINED" };
        }
    }
}

