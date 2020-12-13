#ifndef _MSC_VER
#include "rpc.hpp"
#include "key_value_tree.hpp"
#endif 

#include "sequential_enum.h"

SequentialEnum(RPC_Function_Enum,
		DO_NOTHING,
		RESET_SYSTEM,
		WRITE_KEY_SIZE_VALUE_TO_FLASH,
		ERASE_FLASH_SECTOR,
)

#ifndef _MSC_VER
constexpr uint8_t num_rpc_impl_funcs = RPC_Function_EnumList.size();

struct RPC_State_Data{
	Flash_Key_Value_Tree* flash_key_value_tree;
};

using RPC_Impl = RPC<RPC_State_Data, RPC_Function_Enum, num_rpc_impl_funcs>;
#endif
