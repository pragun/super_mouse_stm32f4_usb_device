#include "stm32f4xx_hal.h"
#include "rpc_impl.hpp"

//Uses the trailing type delcaration format to leave the most flexibility to the user
#define DEF_RPC_FUNC(Y) template<>\
template <>\
auto RPC<RPC_State_Data, RPC_Function_Enum, num_rpc_impl_funcs>::RPC_Function<RPC_Function_Enum::Y>

//extern void HAL_NVIC_SystemReset();
extern void flash_erase();

DEF_RPC_FUNC(DO_NOTHING)(const uint8_t* buf)->void{

}

DEF_RPC_FUNC(RESET_SYSTEM)(const uint8_t* buf)->void{
	HAL_NVIC_SystemReset();
}

DEF_RPC_FUNC(WRITE_KEY_SIZE_VALUE_TO_FLASH)(const uint8_t* buf)->void{
#pragma pack(1)
	struct key_size_value{
		uint32_t key;
		uint8_t size;
		uint8_t value[];
	};

	const key_size_value* ksv = reinterpret_cast<const key_size_value*>(buf);

	state_data.flash_key_value_tree->add_edit_key_value(ksv->key, ksv->size, const_cast<uint8_t*>(ksv->value));
	state_data.flash_key_value_tree->reload();
}

DEF_RPC_FUNC(ERASE_FLASH_SECTOR)(const uint8_t* buf)->void{
	flash_erase();
}


DEF_RPC_FUNC(SET_CURRENT_APPLICATION_ID)(const uint8_t* buf)->void{
	uint8_t app_id = buf[0];
	state_data.mouse_event_handler->set_application_id(app_id);
}


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

template class RPC<RPC_State_Data, RPC_Function_Enum, num_rpc_impl_funcs>;
