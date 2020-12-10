#include "functional"
#include "rpc.hpp"

template<uint8_t i> bool RPC::dispatch_init( RPC::RPC_fptr* pTable ) {
  pTable[ i ] = &RPC::RPC_Function<static_cast<RPC_Function_Enum>(i)>;
  return dispatch_init< i - 1 >( pTable );
}

// edge case of recursion
template<> bool RPC::dispatch_init<-1>( RPC::RPC_fptr* pTable ) { return true; }
// call the recursive function

template<uint8_t i> constexpr std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> RPC::dispatch_init2( std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> &arr ) {
  arr[ i ] = &RPC::RPC_Function<static_cast<RPC_Function_Enum>(i)>;
  return dispatch_init< i - 1 >( arr );
}

// edge case of recursion
template<>  constexpr std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> RPC::dispatch_init2<-1>( std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> &arr) { return arr; }


void RPC::do_nothing(const uint8_t* buf){
}

template <>
void RPC::RPC_Function<RPC_Function_Enum::DO_NOTHING>(const uint8_t* buf){

}

template <>
void RPC::RPC_Function<RPC_Function_Enum::WRITE_KEY_SIZE_VALUE_TO_FLASH>(const uint8_t* buf){
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

template <>
void RPC::RPC_Function<RPC_Function_Enum::ERASE_FLASH_SECTOR>(const uint8_t* buf){

}


//#define DEF_RPC_Func(X) template<> \
//	RPC::RPC_Function<RPC_Function_Enum::X>
//
//
//void DEF_RPC_Func(DO_NOTHING)(const uint8_t* buf){
//
//}

void RPC::Handle_RPC(const uint8_t* buf){
	uint8_t rpc_func_idx = buf[0];
	//rpc_func_list[rpc_func_idx](&buf[1]);
	//RPC_Function<(RPC_Function_Enum) rpc_func_idx>(&buf[1]);
	//std::invoke(rpc_func_list[rpc_func_idx], this, &buf[1]);
}

template<uint8_t idx>
constexpr RPC::RPC_fptr RPC::get_fptr_from_idx(){
	return &RPC::RPC_Function<static_cast<RPC_Function_Enum>(idx)>;
}

template <size_t... Indices>
constexpr std::array<RPC::RPC_fptr, MAX_RPC_FUNCS>
RPC::func_idx_helper(std::index_sequence<Indices...>) {
    return { get_fptr_from_idx<Indices>()... };
}

constexpr std::array<RPC::RPC_fptr, MAX_RPC_FUNCS>
RPC::func_idx_builder() {
    return func_idx_helper(
        // make the sequence type sequence<0, 1, 2, ..., N-1>
        std::make_index_sequence<MAX_RPC_FUNCS>{}
        );
}


RPC::RPC(Flash_Key_Value_Tree* flash_key_value_tree):
flash_key_value_tree(flash_key_value_tree),
func_idx(RPC::func_idx_builder())
{
//	for(uint8_t i =0; i<MAX_RPC_FUNCS; i++){
//		rpc_func_list[i] = &RPC::RPC_Function<RPC_Function_Enum::DO_NOTHING>;
//	}
	const bool initialized = dispatch_init< MAX_RPC_FUNCS-1 >( rpc_func_list );
	//onstexpr std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> a = {&RPC::do_nothing};
	constexpr std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> a = {&RPC::RPC_Function<RPC_Function_Enum::DO_NOTHING>};
	//constexpr std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> b = dispatch_init2< MAX_RPC_FUNCS-1 >(a);
	 std::array<RPC::RPC_fptr,MAX_RPC_FUNCS> b = func_idx_builder();
}

