#pragma bitfield_order left_to_right

enum UsagePage  : u8 {
	INVALID,
	GENERIC_DESKTOP,
	SIMULATION_CONTROLS,
	VR_CONTROLS,
	SPORT_CONTROLS,
	GAME_CONTROLS,
	GENERIC_DEVICE_CONTROLS,
	KEYBOARD,
	BUTTON,
	ORDINAL,
	TELEPHONY_DEVICE,
	CONSUMER,
	DIGITIZER,
	HAPTICS,
	PHYSICAL_INTERFACE,
	UNICODE
};

enum Length : u8 {
	ONE = 0x01,
	TWO = 0x02,
	FOUR = 0x03
};

enum RDItemType : u8 {
	MAIN = 0x00,
	GLOBAL = 0x01,
	LOCAL = 0x02,
};


bitfield ItemHeader {
	tag: 4;
	type: 2;
	length: 2;
};


union RDShortItem {
	ItemHeader header;
	
	u8 values[header.length];
};


RDShortItem item @ 0x00;

//
// 0000 0101
// ^^^^ ^.^-- LENGTH
//  \/   |
// FUNC  `- TYPE
