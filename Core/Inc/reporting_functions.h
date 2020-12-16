
#include "sequential_enum.h"

#define NUM_KEYS_KEYPAD 15 // 12 keys are standard on most MMO mouse. 0 for no key pressed
						   // + Ring finger button, which over-rides the keypad for the application
						   // + Middle mouse button, which also over-rides the keypad for the application

// NO_KEY(0)
// KEYPAD_KEY_RANGE(1-12)
#define SCROLL_BUTTON_KEYPAD_IDX 13
#define RING_BUTTON_KEYPAD_IDX 14

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

SequentialEnum(ReportingFunctionEnum,
	NO_REPORT,
	RELEASE_ALL_KEYS,
	ALTERED_MOUSE_MOVEMENT,
	ABSOLUTE_MOUSE_POSITIION,
	KEYBOARD_PRESS_RELEASE,
	MOTION_MOD_KEY_PRESS_RELEASE,
	MODIFIER_HOLD_PRESS_RELEASE,
	MOTION_AS_SCROLL,
);

template <ReportingFunctionEnum>
struct Reporting_Func_Params_Typedef{

};

#define REPORT_FUNC_PARAMS(X,Y) template<>\
							struct Reporting_Func_Params_Typedef<ReportingFunctionEnum::X>Y


REPORT_FUNC_PARAMS(MOTION_AS_SCROLL,{
		int16_t x_divisor;
		int16_t y_divisor;
	});


REPORT_FUNC_PARAMS(NO_REPORT,{});


REPORT_FUNC_PARAMS(ALTERED_MOUSE_MOVEMENT,{
	int16_t x_factor;
	int16_t y_factor;
	int16_t z_factor;
});

REPORT_FUNC_PARAMS(ABSOLUTE_MOUSE_POSITIION,{
	uint16_t x;
	uint16_t y;
	uint8_t button;
});


REPORT_FUNC_PARAMS(KEYBOARD_PRESS_RELEASE,{
	uint8_t modifier_keys;
	uint8_t keys[3];
});

using keyboard_press_release_params = Reporting_Func_Params_Typedef<ReportingFunctionEnum::KEYBOARD_PRESS_RELEASE>;

REPORT_FUNC_PARAMS(MODIFIER_HOLD_PRESS_RELEASE,{
	uint8_t modifier_hold_state;
	keyboard_press_release_params press_release;
});


REPORT_FUNC_PARAMS(MOTION_MOD_KEY_PRESS_RELEASE,{
	uint8_t x_divisor;
	uint8_t y_divisor;
	uint8_t z_divisor;
	keyboard_press_release_params x_movement_keys[2];
	keyboard_press_release_params y_movement_keys[2];
	keyboard_press_release_params z_movement_keys[2];
});

struct empty_struct{};

struct Keypad_Event_Table{
	uint8_t entry_offsets[NUM_EVENT_TYPES_KEYPAD]; //The event entry boundary for the first one is right at the end of this array
	uint8_t payload[];
};

struct Keypad_Event_Table_Entry_Typedef {
	uint8_t reporting_function_index;
	uint8_t parameter_struct_size;
	uint8_t parameters[];
};

