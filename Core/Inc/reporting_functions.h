#include <array>
#include <algorithm>

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define SequentialEnum(Name,...) \
enum Name { __VA_ARGS__ }; \
namespace \
{ \
    constexpr std::array<Name, NUMARGS(__VA_ARGS__)> Name##List { __VA_ARGS__ }; \
};

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
	uint8_t entry_offsets[NUM_EVENT_TYPES_KEYPAD-1]; //The event entry boundary for the first one is right at the end of this array
	uint8_t payload[];
};

struct Keypad_Event_Table_Entry_Typedef {
	uint8_t reporting_function_index;
	uint8_t parameter_struct_size;
	uint8_t parameters[];
};

#define CFG_ENTRY_SERIALIZATION_SIZE_LIMIT 60

template <ReportingFunctionEnum report_type>
std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> C(Reporting_Func_Params_Typedef<report_type> obj){
	std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> a;
	a[0] = (uint8_t) report_type;
	uint8_t siz = (uint8_t) sizeof(Reporting_Func_Params_Typedef<report_type>);
	a[1] = siz;
	uint8_t* ptr = reinterpret_cast<uint8_t*>(&obj);
	for (uint8_t i = 0; i < siz; i++) {
		a[i + 2] = ptr[i];
	}
	return a;
}


struct User_Config_TypeDef{
	uint8_t application_id;
	uint8_t keypad_key;
	std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> key_down;
	std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> mouse_movement;
	std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> short_release;
	std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> long_release;
};


#define CFG(X, ...) C(Reporting_Func_Params_Typedef<ReportingFunctionEnum::X>__VA_ARGS__)

/*
User_Config_TypeDef list_of_config_key_values[] =
{
		{
				.application_id = 0,
				.keypad_key = 0,
				.key_down = CFG(NO_REPORT,{}),
				.mouse_movement = CFG(ALTERED_MOUSE_MOVEMENT, 
					{.x_factor = 1,
					.y_factor = 1,
					.z_factor = 1, }),
				.short_release = CFG(NO_REPORT,{}),
				.long_release = CFG(NO_REPORT,{}),
		},
};
*/

// Read HID Report from the computer
// cmd = byte 1
// if cmd == RPC:
// 		do_RPC(data)

// do_RPC(){
// lookup RPC function
// index = Data[1,2]
// RPC_Func = RPC_index[index]
// RPC_Func(data[3:])
//

//write_to_flash(key,length,value);






	/* Some other previously tried configuration DSL prototypes
	 * */
	/*
	using no_report = Reporting_Func_Params_Typedef<ReportingFunctionEnum::NO_REPORT>;
	using altered_mouse_movement = Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT>;

	{
						0,
						0,
						C(no_report{}),
						C(altered_mouse_movement{
							.x_factor = 1,
							.y_factor = 1,
							.z_factor = 1, }),
						C(no_report{}),
						C(no_report{}),
				},

							{
							.application_id = 0,
							.keypad_key = 0,
							.key_down = C(no_report{}),
							.mouse_movement = C(
								 altered_mouse_movement{
									.x_factor = 1,
									.y_factor = 1,
									.z_factor = 1, }),
							.short_release = C(no_report{}),
							.long_release = C(no_report{}),
				},

				{
						0,
						0,
						CFG(NO_REPORT,{}),
						CFG(ALTERED_MOUSE_MOVEMENT, {
							.x_factor = 1,
							.y_factor = 1,
							.z_factor = 1, }),
						CFG(NO_REPORT,{}),
						CFG(NO_REPORT,{}),
				},
	}

	*/
