#include <key_value_tree.hpp>

/*
	Key_Value_Store Internal operations:
		- load values, can find and mark the growth_node, and the furthest_occupied_memory_location
				- This needs a traversal_map that can introduce outside changes, and will visit each node
				- The finding of the growth_node will be tied to this

		- add new <Key, Value>:
				- First Make sure that the said key doesn't already exist'
				- Find the growth node
				- Create a RAM copy of the new node
				{
					- Find the starting_location_of_unwritten_flash
					- Add a link from the growth node to the new node
					(These two need to be done together because the second)
					(one depends on the first one and vice versa)
				}
				- Write the node to flash at starting_location_of_unwritten_flash

		- edit <Key, Value> to new value:
			- Find the Node corresponding to the Key,
			- Find the Node corresponding to the parent of the Key Node
			- Find the growth_node -> use the already found growth_node
			- Find the root node -> use the active_root
			- Find the starting_of_unwritten_flash

			- Invalidate the link from the parent to the Key node
			- Invalidate the growth bit for the growth node

			- Create a copy of the Key Node.
				- Mark as "active"root"node"
				- Mark as growth_node
				- Add a link to the root_node (previous root) as a child

			- Write the node to flash
			- Invalidate the "active_root_node" bit
			- Add a root_node_link to the new Key Node
 */

/*: Deprecated, will use growth_node for this
uint32_t furthest_memory_location_used_func(Key_Value_Flash_Node &a, uint32_t previous_max_value){
	return (uint32_t) std::max(previous_max_value, a.furthest_memory_location());
}
*/


template<typename T>
bool always_true(T a){
	return true;
}

template<typename T>
bool always_false(T a){
	return false;
}

Flash_Key_Value_Tree::Flash_Key_Value_Tree(Node_Address address)
{
	active_root = find_active_root_node(address);
	//growth_node = find_growth_node();
};

template <typename T>
typename traversal<T>::return_type Flash_Key_Value_Tree::traverse_with_node_function(typename traversal<T>::func_type func, typename traversal<T>::accumulator_type value){
	std::stack<std::tuple<Node_Address,Node_Address>> traversal_stack;
	traversal_stack.push(std::make_tuple(active_root,0));

	while(traversal_stack.size() != 0){
		auto [node_addr,parent_addr] = traversal_stack.top();
		traversal_stack.pop();

		Key_Value_Flash_Node a = Key_Value_Flash_Node::read_node_from_flash(node_addr);

		if constexpr (traversal<T>::traversal_type == Traversal_Type::fold)
						value = func(a,value);

		if constexpr (traversal<T>::traversal_type == Traversal_Type::map)
						func(a);

		if constexpr (traversal<T>::traversal_type == Traversal_Type::search)
						if(func(a)){
							return std::make_tuple(node_addr, parent_addr);
						};

		for(const auto child_node_addr : a.valid_children()){
			if(child_node_addr != 0){
				traversal_stack.push(std::make_tuple(child_node_addr,node_addr));
			}
		}
	}

	if constexpr(traversal<T>::traversal_type != Traversal_Type::map)
						return value;
}

Node_Address Flash_Key_Value_Tree::find_active_root_node(Node_Address address){
	Node_Address next_address = address;
	bool found_root = false;
	do{
		{
			address = next_address;
			Key_Value_Flash_Node node = Key_Value_Flash_Node::read_node_from_flash(address);
			found_root = node.is_root();
			next_address = node.next_root_node_link();
		} // This extra layer of scope makes sure that the node
		// object created in the last loop iteration is taken off the stack
	}while(!found_root);
	return address;
}

bool Flash_Key_Value_Tree::active_root_found(){
	return (active_root != 0);
}

void Flash_Key_Value_Tree::map_with_node_function(std::function<void(Key_Value_Flash_Node&)>func){
	traverse_with_node_function<void>(func,0);
}

void Flash_Key_Value_Tree::map_with_key_value_function(std::function<void(const uint32_t key, const uint8_t size, const uint8_t* value)> func){
	auto node_function = [func](const Key_Value_Flash_Node &a){
		func(a.key(), a.value_size(), (const unsigned char*) a.value());
	};
	map_with_node_function(node_function);
}

std::tuple<Node_Address, Node_Address> Flash_Key_Value_Tree::find_node_and_parent_matching_key(uint32_t key){
	auto find_function = [key](Key_Value_Flash_Node &a) -> bool {
		return (a.key() == key);
	};

	return traverse_with_node_function<bool>(find_function, std::make_tuple((Node_Address)0,(Node_Address)0));
}


std::tuple<Node_Address, Node_Address> Flash_Key_Value_Tree::find_growth_node(){
	/*
	 * auto find_function = [](Key_Value_Flash_Node &a) -> bool {
		return (a.flag_state<Validity_Flag_Enum::growth_node>());
		};
	 */
	return traverse_with_node_function<bool>(Key_Value_Flash_Node::growth_node_find_func, std::make_tuple((Node_Address)0,(Node_Address)0));
}

bool Flash_Key_Value_Tree::update_key_value(const uint32_t key, const uint8_t size, const uint8_t* value){

}


bool Flash_Key_Value_Tree::add_key_value(uint32_t key, uint8_t size, uint8_t* value){
	/*
	- add new <Key, Value>:
				- First Make sure that the said key doesn't already exist'
				- Find the growth node
				- Create a RAM copy of the new node
				{
					- Find the starting_location_of_unwritten_flash
					- Add a link from the growth node to the new node
					(These two need to be done together because the second)
					(one depends on the first one and vice versa)
				}
				- Write the node to flash at starting_location_of_unwritten_flash
	*/

	auto [node_addr, parent_addr] = find_node_and_parent_matching_key(key);
	if(node_addr != 0){
		return false; // This can be a bit more sophisticated
	}

	auto [growth_node_addr,__node_addr1] = find_growth_node();

	auto growth_node = Key_Value_Flash_Node::read_node_from_flash(growth_node_addr);

	Node_Address new_node_addr = growth_node.add_growth_link();

	Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_growth_node(key, size, value);

	new_node.write_to_address(new_node_addr);

}

