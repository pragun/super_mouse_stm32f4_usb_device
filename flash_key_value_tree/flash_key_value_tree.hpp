#include <cstdio>
#include <stack>
#include <tuple>
#include <functional>
#include "flash_key_value_node.hpp"

using useless_type = int;

enum class Traversal_Type {fold, search, map};

template <typename FT>
struct traversal{
	typedef std::function<FT(Key_Value_Flash_Node&, FT)> func_type;
	typedef FT return_type;
	typedef return_type accumulator_type;
	constexpr static Traversal_Type traversal_type = Traversal_Type::fold;
};

template<>
struct traversal<bool>{ //The case of a searching type function
	typedef std::function<bool(Key_Value_Flash_Node&)> func_type;
	typedef std::tuple<Node_Address,Node_Address> return_type;
	typedef return_type accumulator_type;
	constexpr static Traversal_Type traversal_type = Traversal_Type::search;
};

template<>
struct traversal<void>{
	typedef std::function<void(Key_Value_Flash_Node&)> func_type;
	typedef void return_type;
	typedef int accumulator_type;
	constexpr static Traversal_Type traversal_type = Traversal_Type::map;
};

class Flash_Key_Value_Store { //Rename to Flash_Key_Value_Tree
private:
	Node_Address active_root;
	Node_Address growth_node;
	uint32_t furthest_used_memory_location;

public:
	Flash_Key_Value_Store(Node_Address address);

	template <typename T>
	typename traversal<T>::return_type traverse_with_node_function(typename traversal<T>::func_type func, typename traversal<T>::accumulator_type value);

	static Node_Address find_active_root_node(Node_Address address);

	bool active_root_found();

	//If FT is void, then treat it as if it is a map
	//If FT is bool, then treat it as if it search and return std::tuple<Node_Address,Node_Address>
	//If FT is T, then treat it as if T is the result of a fold

	void map_with_node_function(std::function<void(Key_Value_Flash_Node&)>func);

	void map_with_key_value_function(std::function<void(const uint32_t key, const uint8_t size, const uint8_t* value)> func);

	uint32_t find_furthest_used_memory_location();

	std::tuple<Node_Address, Node_Address> find_node_and_parent_matching_key(uint32_t key);

	std::tuple<Node_Address, Node_Address> find_growth_node();

	bool update_key_value(const uint32_t key, const uint8_t size, const uint8_t* value);
};

