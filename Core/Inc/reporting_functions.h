#pragma once
#include <array>
#include <algorithm>

#ifdef _MSC_VER
#define SequentialEnum(Name,...) \
enum Name { __VA_ARGS__ }; \

#else
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define SequentialEnum(Name,...) \
enum Name { __VA_ARGS__ }; \
namespace \
{ \
    constexpr std::array<Name, NUMARGS(__VA_ARGS__)> Name##List { __VA_ARGS__ }; \
};
#endif

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

SequentialEnum(ReportingFunctionEnum,
	NO_REPORT,
	ALTERED_MOUSE_MOVEMENT,
	ABSOLUTE_MOUSE_POSITIION,
	KEYBOARD_PRESS_RELEASE,
	MOTION_MOD_KEY_PRESS_RELEASE,
);

template <ReportingFunctionEnum>
struct Reporting_Func_Params_Typedef{

};

template <>
struct Reporting_Func_Params_Typedef<ReportingFunctionEnum::NO_REPORT>{
};


template <>
struct Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT>{
	int16_t x_factor;
	int16_t y_factor;
	int16_t z_factor;
};

template <>
struct Reporting_Func_Params_Typedef<ReportingFunctionEnum::ABSOLUTE_MOUSE_POSITIION>{
	uint16_t x;
	uint16_t y;
	uint8_t button;
};

template <>
struct Reporting_Func_Params_Typedef<ReportingFunctionEnum::KEYBOARD_PRESS_RELEASE>{
	uint8_t modifier_keys;
	uint8_t keys[3];
};

using keyboard_press_release_params = Reporting_Func_Params_Typedef<ReportingFunctionEnum::KEYBOARD_PRESS_RELEASE>;

template <>
struct Reporting_Func_Params_Typedef<ReportingFunctionEnum::MOTION_MOD_KEY_PRESS_RELEASE>{
	uint8_t x_divisor;
	uint8_t y_divisor;
	uint8_t z_divisor;
	keyboard_press_release_params x_movement_keys[2];
	keyboard_press_release_params y_movement_keys[2];
	keyboard_press_release_params z_movement_keys[2];
};

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

