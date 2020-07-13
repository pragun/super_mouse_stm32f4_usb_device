#include <key_value_node.hpp>
#include <cstdio>
#include <stack>
#include <tuple>
#include <functional>

using useless_type = int;

enum class Traversal_Type {fold, search, map};

template <typename FT>
struct traversal_types{
	typedef std::function<FT(Key_Value_Flash_Node&, FT)> func;
	typedef FT return_t;
	typedef return_t accumulator;
	constexpr static Traversal_Type handling = Traversal_Type::fold;
};

template<>
struct traversal_types<bool>{ //The case of a searching type function
	typedef std::function<bool(Key_Value_Flash_Node&)> func;
	typedef std::tuple<Node_Address,Node_Address> return_t;
	typedef return_t accumulator;
	constexpr static Traversal_Type handling = Traversal_Type::search;
};

template<>
struct traversal_types<void>{
	typedef std::function<void(Key_Value_Flash_Node&)> func;
	typedef void return_t;
	typedef int accumulator;
	constexpr static Traversal_Type handling = Traversal_Type::map;
};

enum class Tree_State : uint8_t {rooted = 0};

class Flash_Key_Value_Tree {
private:
	uint32_t starting_address;
	Node_Address active_root_node_addr;
	uint32_t* status;

	bool add_growth_node_with_key_value(uint32_t key, uint8_t size, uint8_t* value);
	bool add_first_root_node(uint32_t key, uint8_t size, uint8_t* value);
	bool update_node(const uint32_t key, const uint8_t size, const uint8_t* value, Node_Address node_addr, Node_Address parent_addr);

	template <typename T>
	typename traversal_types<T>::return_t traverse_with_node_function(typename traversal_types<T>::func func, typename traversal_types<T>::accumulator value);
	//If FT is void, then treat it as if it is a map
	//If FT is bool, then treat it as if it search and return std::tuple<Node_Address,Node_Address>
	//If FT is T, then treat it as if T is the result of a fold

	static Node_Address find_active_root_node(Node_Address address);
	bool active_root_found();
	Node_Address get_active_root_node_address();
	void map_with_node_function(std::function<void(Key_Value_Flash_Node&)>func);
	void map_with_key_value_function(std::function<void(const uint32_t key, const uint8_t size, const uint8_t* value)> func);

	template <Tree_State TS>
	bool state();

public:
	Flash_Key_Value_Tree(Node_Address address);

	std::tuple<Node_Address, Node_Address> find_node_and_parent_matching_key(uint32_t key);
	std::tuple<Node_Address, Node_Address> find_growth_node();
	bool add_edit_key_value(uint32_t key, uint8_t size, uint8_t* value);
};
