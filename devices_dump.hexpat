struct PascalWString {
    u8 length;
    char16 string[length];
};

struct Input {

};

struct Device {
    u16 vendor_id;
    u16 product_id;
    u32 bus;
    PascalWString manufacturer;
    PascalWString product;
    PascalWString serial;
    
    u8 inputs_count;
    Input inputs[inputs_count];
};   

char magic[4] @ 0x00;
u8 version @ $;
u8 count @ $;
Device devices[count] @ $;