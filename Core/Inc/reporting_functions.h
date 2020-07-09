#define NUM_KEYS_KEYPAD 12 // 12 keys are standard on most MMO mouse
#define NUM_EVENT_TYPES_KEYPAD 3 // Short Press, LongPress and Movement
#define NUM_APPLICATIONS_KEYPAD 16 // Can store between 16 application specific reporting options

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

struct Keypad_Lookup_Redirect_Entry_Typedef{
	uint8_t redirect_byte;
	uint8_t __empty__;
	uint8_t keypad_num;
	uint8_t application_id;
};

struct Keypad_Event_Table{
	uint8_t num_events;
	uint8_t entry_offsets[];
};

union Keypad_Reporting_Lookup_Entry_Typedef
{
	Keypad_Event_Table* evt_table;
	Keypad_Lookup_Redirect_Entry_Typedef redirect;
};

template<typename payload>
struct Keypad_Event_Table_Entry_Typedef {
	uint8_t reporting_function_index;
	uint8_t parameter_struct_size;
	payload parameters;
};


