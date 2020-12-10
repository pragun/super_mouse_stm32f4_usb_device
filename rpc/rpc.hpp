#import "key_value_tree.hpp"

#import <functional>

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

class RPC{
public:
	constexpr static uint8_t MAX_RPC_FUNCS = RPC_Function_EnumList.size();

	void Handle_RPC(const uint8_t*);
	RPC(Flash_Key_Value_Tree* flash_key_value_tree);

private:


	Flash_Key_Value_Tree* flash_key_value_tree;
	typedef void (RPC::*RPC_fptr)(const uint8_t*);

	template <RPC_Function_Enum T>
	void RPC_Function(const uint8_t*);

	static constexpr std::array<RPC::RPC_fptr, RPC::MAX_RPC_FUNCS>
	func_idx_builder();

	template <size_t... Indices>
	static constexpr std::array<RPC::RPC_fptr, RPC::MAX_RPC_FUNCS>
	func_idx_helper(std::index_sequence<Indices...>);

	template<uint8_t idx>
	static constexpr RPC_fptr get_fptr_from_idx();

	std::array<RPC_fptr, MAX_RPC_FUNCS> func_list;

};
