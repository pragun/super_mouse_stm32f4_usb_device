#import "key_value_tree.hpp"
#define MAX_RPC_FUNCS 255

class HID_RPC{
private:
	Flash_Key_Value_Tree* flash_key_value_tree;
	typedef void (HID_RPC::*RPC_fptr)(const uint8_t*);
	RPC_fptr rpc_func_list[MAX_RPC_FUNCS];

	void dont_do_anything(const uint8_t*);
	void write_key_size_value_to_flash(const uint8_t*);

public:
	void Handle_HID_RPC(const uint8_t*);
	HID_RPC(Flash_Key_Value_Tree* flash_key_value_tree);
};
