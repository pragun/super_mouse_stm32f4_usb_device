#define NUM_KEYS_KEYPAD 12 // 12 keys are standard on most MMO mouse
#define NUM_EVENT_TYPES_KEYPAD 3 // Short Press, LongPress and Movement
#define NUM_APPLICATIONS_KEYPAD 16 // Can store between 16 application specific reporting options


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


//The Key_Report_Function_Config_Typedef
//Data for the three kinds of events
//Is stored at the offsets listed in the event_type array
//The offset add to the &Event_Offset_Table of the struct object
struct EventType_Offset_Table{
	uint8_t __reserved__; //padding to make size 4 bytes
	uint8_t event_table[NUM_EVENT_TYPES_KEYPAD];
};

//The following struct redirects to event offset table stored at a different place
//In case several reports share the same event offset table
struct Redirect_For_Data{

};

struct Key_Report_Flash_Config_Node_Typedef {
	uint8_t redirect: 1, valid: 1, node_ptr_validity_bitmask: 4;
	uint8_t application_index;
	uint8_t keypad_keynum;
	uint8_t node_links_struct_offset;
	EventType_Offset_Table* ptr_to_report_offset_table;
		//If redirect bit is set
};

struct Key_Report_Event_Config_Typedef {
	uint8_t reporting_function_index;
	uint8_t parameter_struct_size;
	uint8_t parameter_ptr[];
};

template<typename payload>
struct Key_Report_Event_Config_Typedef2 {
	uint8_t reporting_function_index;
	uint8_t parameter_struct_size;
	payload parameters;
};
