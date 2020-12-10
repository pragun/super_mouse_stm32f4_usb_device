#import "rpc_impl.hpp"

//Uses the trailing type delcaration format to leave the most flexibility to the user
#define DEF_RPC_FUNC(Y) template<>\
template <>\
auto RPC<RPC_Function_Enum, num_rpc_impl_funcs>::RPC_Function<RPC_Function_Enum::Y>


DEF_RPC_FUNC(DO_NOTHING)(const uint8_t* buf)->void{

}

DEF_RPC_FUNC(WRITE_KEY_SIZE_VALUE_TO_FLASH)(const uint8_t* buf)->void{
#pragma pack(1)
	struct key_size_value{
		uint32_t key;
		uint8_t size;
		uint8_t value[];
	};

	const key_size_value* ksv = reinterpret_cast<const key_size_value*>(buf);

	flash_key_value_tree->add_edit_key_value(ksv->key, ksv->size, const_cast<uint8_t*>(ksv->value));
	flash_key_value_tree->reload();
}

DEF_RPC_FUNC(ERASE_FLASH_SECTOR)(const uint8_t* buf)->void{

}


constexpr uint8_t CFG_ENTRY_SERIALIZATION_SIZE_LIMIT = 60;

struct Cfg_Serialized_Entry_TypeDef{
	uint8_t report_type;
	uint8_t cfg_size;
	std::array<uint8_t, CFG_ENTRY_SERIALIZATION_SIZE_LIMIT> data;
};

struct User_Config_TypeDef{
	uint8_t application_id;
	uint8_t keypad_key;

	Cfg_Serialized_Entry_TypeDef key_down;
	Cfg_Serialized_Entry_TypeDef mouse_movement;
	Cfg_Serialized_Entry_TypeDef short_release;
	Cfg_Serialized_Entry_TypeDef long_release;
};


template <ReportingFunctionEnum report_type>
Cfg_Serialized_Entry_TypeDef C(Reporting_Func_Params_Typedef<report_type> obj){
	Cfg_Serialized_Entry_TypeDef a;
	a.report_type = (uint8_t) report_type;
	uint8_t siz = (uint8_t) sizeof(Reporting_Func_Params_Typedef<report_type>);
	a.cfg_size = siz;

	uint8_t* ptr = reinterpret_cast<uint8_t*>(&obj);
	for (uint8_t i = 0; i < siz; i++) {
		a.data[i + 2] = ptr[i];
	}

	return a;
}

struct key_size_value_flash{
	uint32_t key;
	uint8_t size;
	uint8_t value[CFG_ENTRY_SERIALIZATION_SIZE_LIMIT];
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


key_size_value_flash convert_cfg_entry_to_key_size_value(Cfg_Serialized_Entry_TypeDef cfg){

}


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

template class RPC<RPC_Function_Enum, num_rpc_impl_funcs>;
