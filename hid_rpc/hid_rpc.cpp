#include "hid_rpc.hpp"
#include "functional"

HID_RPC::HID_RPC(Flash_Key_Value_Tree* flash_key_value_tree):
flash_key_value_tree(flash_key_value_tree)
{
	for(uint8_t i =0; i<MAX_RPC_FUNCS; i++){
		rpc_func_list[i] = &HID_RPC::dont_do_anything;
	}

	rpc_func_list[1] = &HID_RPC::write_key_size_value_to_flash;
}

void HID_RPC::dont_do_anything(const uint8_t* buf){

}

void HID_RPC::Handle_HID_RPC(const uint8_t* buf){
	uint8_t rpc_func_idx = buf[0];
	//rpc_func_list[rpc_func_idx](&buf[1]);
	std::invoke(rpc_func_list[rpc_func_idx], this, &buf[1]);
}

void HID_RPC::write_key_size_value_to_flash(const uint8_t* buf){
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


