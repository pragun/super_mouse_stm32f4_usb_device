/*
 * default_config.cpp
 *
 *  Created on: Dec 12, 2020
 *      Author: Pragun
 */

#include "reporting_functions.h"
#include <cstring>

#define DEF_CONFIG 1
#include "default_config.h"

//Default Event Handler
//#define EVT_SIZ(x) (sizeof(x)+ x.parameter_struct_size)
#define EVT_SIZ(x) sizeof(x)

/*
constexpr uint8_t default_cfg_size =
		sizeof(Keypad_Event_Table) +
		sizeof(Keypad_Event_Table_Entry_Typedef) +
		sizeof(Keypad_Event_Table_Entry_Typedef) +
		sizeof(Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT>) +
		sizeof(Keypad_Event_Table_Entry_Typedef) +
		sizeof(Keypad_Event_Table_Entry_Typedef);


This would be the most non-wasteful way to figure out the size, but that leads to complicated
includes, so for now this default_cfg_size is hard-coded in default_config.h
*/

uint8_t default_cfg[default_cfg_size];


#define CPY_2_CFG(itm) std::memcpy(&default_cfg[idx], (uint8_t*)&itm, sizeof(itm));\
					   idx = idx + sizeof(itm);

void fill_default_cfg(){
	uint8_t idx = 0;


	Keypad_Event_Table event_table =
		{	.entry_offsets = {
				0,
				EVT_SIZ(Keypad_Event_Table_Entry_Typedef),
				2*EVT_SIZ(Keypad_Event_Table_Entry_Typedef) + EVT_SIZ(Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT>),
				3*EVT_SIZ(Keypad_Event_Table_Entry_Typedef) + EVT_SIZ(Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT>)},
			.payload = {},
		};

	CPY_2_CFG(event_table);

	Keypad_Event_Table_Entry_Typedef def_key_down =
		{	.reporting_function_index = (uint8_t) ReportingFunctionEnum::NO_REPORT,
			.parameter_struct_size = 0,
			.parameters = {},
		};

	CPY_2_CFG(def_key_down);

	Keypad_Event_Table_Entry_Typedef def_mouse_movement =
		{	.reporting_function_index = (uint8_t) ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT,
			.parameter_struct_size = sizeof(Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT>),
			.parameters = {},
		};

	CPY_2_CFG(def_mouse_movement);

	Reporting_Func_Params_Typedef<ReportingFunctionEnum::ALTERED_MOUSE_MOVEMENT> def_mouse_params =
		{	.x_factor = 1,
			.y_factor = 1,
			.z_factor = 1,
		};

	CPY_2_CFG(def_mouse_params);

	Keypad_Event_Table_Entry_Typedef def_key_short_release =
		{	.reporting_function_index = (uint8_t) ReportingFunctionEnum::NO_REPORT,
			.parameter_struct_size = 0,
			.parameters = {},
		};

	CPY_2_CFG(def_key_short_release);

	Keypad_Event_Table_Entry_Typedef def_key_long_release =
		{	.reporting_function_index = (uint8_t) ReportingFunctionEnum::NO_REPORT,
			.parameter_struct_size = 0,
			.parameters = {},
		};

	CPY_2_CFG(def_key_long_release);
}
