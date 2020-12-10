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

constexpr uint8_t MAX_RPC_FUNCS = RPC_Function_EnumList.size();


class RPC{
private:
	Flash_Key_Value_Tree* flash_key_value_tree;
	typedef void (RPC::*RPC_fptr)(const uint8_t*);


	typedef  std::function<void (const uint8_t*)> RPC_Func;

	template <RPC_Function_Enum T>
	void RPC_Function(const uint8_t*);

	RPC_fptr rpc_func_list[MAX_RPC_FUNCS];


	template< uint8_t i >
	bool dispatch_init( RPC_fptr* pTable );

	template<uint8_t i>
	constexpr std::array<RPC_fptr,MAX_RPC_FUNCS> dispatch_init2( std::array<RPC_fptr,MAX_RPC_FUNCS> &arr);

	void do_nothing(const uint8_t* buf);

	static constexpr std::array<RPC::RPC_fptr, MAX_RPC_FUNCS>
	func_idx_builder();

	template <size_t... Indices>
	static constexpr std::array<RPC::RPC_fptr, MAX_RPC_FUNCS>
	func_idx_helper(std::index_sequence<Indices...>);

	template<uint8_t idx>
	static constexpr RPC_fptr get_fptr_from_idx();

	std::array<RPC_fptr, MAX_RPC_FUNCS> func_idx;

public:
	void Handle_RPC(const uint8_t*);
	RPC(Flash_Key_Value_Tree* flash_key_value_tree);
};

/*
template <typename T, size_t N>
auto func_idx_builder() {
    using Sequence = std::make_integer_sequence<int, N>;
    return func_idx_helper<std::array<T, N>>(Sequence{});
}

template <typename Container, int... I>
Container func_idx_helper(std::integer_sequence<int, I...>) {
    return {&RPC::RPC_Function<static_cast<RPC_Function_Enum>(I)>...};
} */



/*
template <typename Tuple, size_t... Indices>
std::array<int, sizeof...(Indices)>
call_f_detail(Tuple& tuple, std::index_sequence<Indices...> ) {
    return { f(std::get<Indices>(tuple))... };
}

template <typename Tuple>
std::array<int, std::tuple_size<Tuple>::value>
call_f(Tuple& tuple) {
    return call_f_detail(tuple,
        // make the sequence type sequence<0, 1, 2, ..., N-1>
        std::make_index_sequence<std::tuple_size<Tuple>::value>{}
        );
}
*/
