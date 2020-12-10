#import "key_value_tree.hpp"


template<typename Func_Enum, uint8_t num_funcs>
#define RPC_Class_Def typename RPC<Func_Enum,num_funcs>
class RPC{
public:
	//constexpr static uint8_t NUM_RPC_FUNCS = RPC_Function_EnumList.size();
	constexpr static uint8_t NUM_RPC_FUNCS = num_funcs;

	void Handle_RPC(const uint8_t*);
	RPC(Flash_Key_Value_Tree* flash_key_value_tree);


private:


	Flash_Key_Value_Tree* flash_key_value_tree;
	typedef void (RPC::*RPC_fptr)(const uint8_t*);

	template <Func_Enum T>
	void RPC_Function(const uint8_t*);

	static constexpr std::array<RPC_Class_Def::RPC_fptr, num_funcs>
	func_idx_builder();

	template <size_t... Indices>
	static constexpr std::array<RPC_Class_Def::RPC_fptr, num_funcs>
	func_idx_helper(std::index_sequence<Indices...>);

	template<uint8_t idx>
	static constexpr RPC_fptr get_fptr_from_idx();

	std::array<RPC_fptr, NUM_RPC_FUNCS> func_list;

};

template<typename Func_Enum, uint8_t num_funcs>
void RPC<Func_Enum, num_funcs>::Handle_RPC(const uint8_t* buf){
	uint8_t rpc_func_idx = buf[0];
	std::invoke(func_list[rpc_func_idx], this, &buf[1]);
}

template<typename Func_Enum, uint8_t num_funcs>
template<uint8_t idx>
constexpr RPC_Class_Def::RPC_fptr RPC<Func_Enum, num_funcs>::get_fptr_from_idx(){
	return &RPC<Func_Enum, num_funcs>::RPC_Function<static_cast<Func_Enum>(idx)>;
}

template<typename Func_Enum, uint8_t num_funcs>
template <size_t... Indices>
constexpr std::array<RPC_Class_Def::RPC_fptr, num_funcs>
RPC<Func_Enum, num_funcs>::func_idx_helper(std::index_sequence<Indices...>) {
    return { get_fptr_from_idx<Indices>()... };
}

template<typename Func_Enum, uint8_t num_funcs>
constexpr std::array<RPC_Class_Def::RPC_fptr, num_funcs>
RPC<Func_Enum, num_funcs>::func_idx_builder() {
    return func_idx_helper(
        // make the sequence type sequence<0, 1, 2, ..., N-1>
        std::make_index_sequence<NUM_RPC_FUNCS>{}
        );
}

template<typename Func_Enum, uint8_t num_funcs>
RPC<Func_Enum, num_funcs>::RPC(Flash_Key_Value_Tree* flash_key_value_tree):
flash_key_value_tree(flash_key_value_tree),
func_list(RPC::func_idx_builder())
{
}
