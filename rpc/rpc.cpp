#include "functional"
#include "rpc.hpp"


//Uses the trailing type delcaration format to leave the most flexibility to the user
#define DEF_RPC_FUNC(Y) template <>\
auto RPC::RPC_Function<RPC_Function_Enum::Y>


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


void RPC::Handle_RPC(const uint8_t* buf){
	uint8_t rpc_func_idx = buf[0];
	std::invoke(func_list[rpc_func_idx], this, &buf[1]);
}

template<uint8_t idx>
constexpr RPC::RPC_fptr RPC::get_fptr_from_idx(){
	return &RPC::RPC_Function<static_cast<RPC_Function_Enum>(idx)>;
}

template <size_t... Indices>
constexpr std::array<RPC::RPC_fptr, RPC::MAX_RPC_FUNCS>
RPC::func_idx_helper(std::index_sequence<Indices...>) {
    return { get_fptr_from_idx<Indices>()... };
}

constexpr std::array<RPC::RPC_fptr, RPC::MAX_RPC_FUNCS>
RPC::func_idx_builder() {
    return func_idx_helper(
        // make the sequence type sequence<0, 1, 2, ..., N-1>
        std::make_index_sequence<MAX_RPC_FUNCS>{}
        );
}

RPC::RPC(Flash_Key_Value_Tree* flash_key_value_tree):
flash_key_value_tree(flash_key_value_tree),
func_list(RPC::func_idx_builder())
{
}

