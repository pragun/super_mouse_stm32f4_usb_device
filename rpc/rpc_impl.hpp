#import "rpc.hpp"
#import "key_value_tree.hpp"
#import "reporting_functions.h"


#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define SequentialEnum(Name,...) \
enum Name { __VA_ARGS__ }; \
namespace \
{ \
    constexpr std::array<Name, NUMARGS(__VA_ARGS__)> Name##List { __VA_ARGS__ }; \
};

SequentialEnum(RPC_Function_Enum,
		DO_NOTHING,
		WRITE_KEY_SIZE_VALUE_TO_FLASH,
		ERASE_FLASH_SECTOR,
)

constexpr uint8_t num_rpc_impl_funcs = RPC_Function_EnumList.size();

struct RPC_State_Data{
	Flash_Key_Value_Tree* flash_key_value_tree;
};
