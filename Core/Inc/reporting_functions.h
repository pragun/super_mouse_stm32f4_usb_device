#define NUM_KEYS_KEYPAD 13 // 12 keys are standard on most MMO mouse. 0 for no key pressed
#define NUM_EVENT_TYPES_KEYPAD 4 // Key_Press, Short_Press_Release, LongPress_Release and Movement
#define NUM_APPLICATIONS_KEYPAD 16 // Can store between 16 application specific reporting options

namespace ReportingEventTypes{
	enum ReportingEventTypes:uint8_t{
		KEY_DOWN = 0,
		MOUSE_MOVEMENT,
		SHORT_PRESS_UP,
		LONG_PRESS_UP,
	};
}

#include <array>
#include <algorithm>

struct Altered_Mouse_Movement_Params_Typedef{
	int16_t x_factor;
	int16_t y_factor;
	int16_t z_factor;
};

struct Absolute_Mouse_Params_Typedef{
	uint16_t x;
	uint16_t y;
	uint8_t button;
};

struct Keyboard_Press_Release_Params_Typedef{
	uint8_t modifier_keys;
	uint8_t keys[3];
};

struct empty_struct{};


namespace ReportingFunctionIndex {
	enum ReportingFunctionIndex : uint8_t {
		NO_REPORT = 0,
		ALTERED_MOUSE_MOVEMENT,
		ABSOLUTE_MOUSE_POSITIION,
		KEYBOARD_PRESS_RELEASE,
		MOTION_MOD_KEY_PRESS_RELEASE,
	};
}

struct Keypad_Event_Table{
	uint8_t entry_offsets[NUM_EVENT_TYPES_KEYPAD-1]; //The event entry boundary for the first one is right at the end of this array
	uint8_t payload[];
};


struct Keypad_Event_Table_Entry_Typedef {
	uint8_t reporting_function_index;
	uint8_t parameter_struct_size;
	uint8_t parameters[];
};


