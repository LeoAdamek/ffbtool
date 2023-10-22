#include <stdio.h>
#include "hid.hxx"
#include "hid_descriptor.hxx"

void save_devices(const char *path) {
    FILE *file = fopen(path, "w");

    if (file)
    {
        fprintf(file, "FFBT");
        fputc(0x01, file);
        uint8_t sz = 0;
        const hid_device_info *device = HID::GlobalDeviceManager.get_devices();

        for (; device; device = device->next)
            sz++;
        fwrite(&sz, sizeof(sz), 1, file);

        device = HID::GlobalDeviceManager.get_devices();
        for (; device; device = device->next)
        {
            fwrite(&device->vendor_id, sizeof(device->vendor_id), 1, file);
            fwrite(&device->product_id, sizeof(device->product_id), 1, file);
            fwrite(&device->bus_type, sizeof(device->bus_type), 1, file);

            sz = wcsnlen(device->manufacturer_string, -1);
            fwrite(&sz, sizeof(sz), 1, file);
            if (sz)
                fwrite(device->manufacturer_string, sizeof(wchar_t), sz, file);

            sz = wcsnlen(device->product_string, -1);
            fwrite(&sz, sizeof(sz), 1, file);
            if (sz)
                fwrite(device->product_string, sizeof(wchar_t), sz, file);

            sz = wcsnlen(device->serial_number, -1);
            fwrite(&sz, sizeof(sz), 1, file);
            if (sz)
                fwrite(device->serial_number, sizeof(wchar_t), sz, file);

            auto dev = HID::GlobalDeviceManager.get_device(device);
            auto descr = HID::Descriptor::parse(dev->report_descriptor.data, dev->report_descriptor.length);

            sz = descr.inputs.size();
            fwrite(&sz, sizeof(sz), 1, file);
            for (auto input : descr.inputs)
            {
                fwrite(&input, sizeof(HID::Descriptor::Node), 1, file);
            }

            sz = descr.outputs.size();
            fwrite(&sz, sizeof(sz), 1, file);
            for (auto output : descr.outputs)
            {
                fwrite(&output, sizeof(HID::Descriptor::Node), 1, file);
            }

            sz = descr.features.size();
            fwrite(&sz, sizeof(sz), 1, file);
            for (auto feature : descr.features)
            {
                fwrite(&feature, sizeof(HID::Descriptor::Node), 1, file);
            }

            fflush(file);
        }

        fclose(file);
    }
}