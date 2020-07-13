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
	starting_address = address;
	status = (uint32_t*) address;

	//active_root_node_addr = find_active_root_node(address);
	//growth_node = find_growth_node();
};


template <typename T>
typename traversal_types<T>::return_t Flash_Key_Value_Tree::traverse_with_node_function(typename traversal_types<T>::func func, typename traversal_types<T>::accumulator value){
	std::stack<std::tuple<Node_Address,Node_Address>> traversal_stack;
	traversal_stack.push(std::make_tuple(active_root_node_addr,0));

	while(traversal_stack.size() != 0){
		auto [node_addr,parent_addr] = traversal_stack.top();
		traversal_stack.pop();

		Key_Value_Flash_Node a = Key_Value_Flash_Node::read_node_from_flash(node_addr);

		if constexpr (traversal_types<T>::handling == Traversal_Type::fold)
						value = func(a,value);

		if constexpr (traversal_types<T>::handling == Traversal_Type::map)
						func(a);

		if constexpr (traversal_types<T>::handling == Traversal_Type::search)
						if(func(a)){
							return std::make_tuple(node_addr, parent_addr);
						};

		for(const auto child_node_addr : a.valid_children_links()){
			if(child_node_addr != 0){
				traversal_stack.push(std::make_tuple(child_node_addr,node_addr));
			}
		}
	}

	if constexpr(traversal_types<T>::handling != Traversal_Type::map)
						return value;
}

template <Tree_State T>
bool Flash_Key_Value_Tree::state(){
	return ! ( (bool) ((*status) & (0x1 << static_cast<uint8_t>(T))) );
}

/* Use this when some of the bits are to be read inverted
template <>
bool Flash_Key_Value_Tree::state<Tree_State::rooted>(){
	return !( (bool) ((*status) & (0x1 << static_cast<uint8_t>(Tree_State::rooted))) );
} */

Node_Address Flash_Key_Value_Tree::find_active_root_node(Node_Address address){
	Node_Address next_address = address;
	bool found_root = false;
	do{
		{
			address = next_address;
			Key_Value_Flash_Node node = Key_Value_Flash_Node::read_node_from_flash(address);
			found_root = node.is_root();
			next_address = node.link_to_next_root_node();
		} // This extra layer of scope makes sure that the node
		// object created in the last loop iteration is taken off the stack
	}while(!found_root);
	return address;
}

bool Flash_Key_Value_Tree::active_root_found(){
	return (active_root_node_addr != 0);
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


bool Flash_Key_Value_Tree::update_node(const uint32_t key, const uint8_t size, const uint8_t* value, Node_Address edit_node_addr, Node_Address parent_addr){
	/*	Edit <Key, Value> to new value:
			- Find the existing Node corresponding to the Key [edit_node]
			- Find the Node corresponding to the parent of the edit_node [edit_nodes_parent_node]
			- Find the growth_node -> use the already found growth_node [growth_node]
			- Find the root node -> use the active_root [active_root_node]
			- Find the starting_of_unwritten_flash

			- Invalidate the growth bit for the growth node
			- Invalidate the valid_data bit for the edit_node
			- Invalidate the active_root_node for the root node

			- Invalidate the link from the parent to the Key node [[needs more thought]]

			- Create a copy of the Key Node.
				- Mark as "active"root"node"
				- Mark as growth_node
				- Add a link to the root_node (previous root) as a child

			- Write the node to flash
			- Invalidate the "active_root_node" bit
			- Add a root_node_link to the new Key Node */

	{
		auto [growth_node_addr,__node_addr1] = find_growth_node();
		auto growth_node = Key_Value_Flash_Node::read_node_from_flash(growth_node_addr);
		growth_node.mark_growth_link_as_invalid();
	}

	Key_Value_Flash_Node edit_node = Key_Value_Flash_Node::read_node_from_flash(edit_node_addr);
	edit_node.mark_data_as_invalid();

	auto active_root_node_addr = get_active_root_node_address();
	Key_Value_Flash_Node&& active_root_node = (active_root_node_addr == edit_node_addr)? edit_node : Key_Value_Flash_Node::read_node_from_flash(edit_node_addr);
	active_root_node.mark_active_root_as_invalid();

	Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_root_node(key, size, value);

	uint8_t max_num_links_that_can_be_transferred_in_from_old_node = 0;
	// Ideally, the new root_node should take care of including all valid links from the previous node being edited
	// + a link to the old root node, + leaving space for growth_nodes
	// So the maximum number of links that it can take from the previous node are:
	// MAX_NUM_LINKS - 2
	//If the number of links exceed this on the previous node, then the previous node will be left in
	if(active_root_node_addr == edit_node_addr){
		max_num_links_that_can_be_transferred_in_from_old_node = MAX_NUM_CHILD_NODES - 2;
		new_node.add_valid_non_growth_child_link(active_root_node.flash_address());
	}else{
		// Special case:
		// The active root is also the edit node, in which case no extra link is needed to
		// point back to the root,
		// So the maximum number of links that it can take from the previous node are:
		// MAX_NUM_LINKS - 1
		max_num_links_that_can_be_transferred_in_from_old_node = MAX_NUM_CHILD_NODES - 1;
	}

	if(edit_node.num_valid_children_links() <= max_num_links_that_can_be_transferred_in_from_old_node){

		std::array<Node_Address,MAX_NUM_CHILD_NODES> edit_nodes_children = edit_node.valid_children_links();
		for(const auto edit_nodes_child : edit_nodes_children){
			if(edit_nodes_child != (Node_Address)0){
				new_node.add_valid_non_growth_child_link(edit_nodes_child);
			}
		}

		//We'll mark the link from the parent node pointing to the edit_node as invalid
		//as the new node will take care of the responsibility for each of the edit node's children
		if(parent_addr != (Node_Address) 0){
				auto edit_nodes_parent_node = Key_Value_Flash_Node::read_node_from_flash(parent_addr);
				edit_nodes_parent_node.mark_invalid_link_to(edit_node_addr);
		}
	}else{
		// We wouldn't invalidate the link from the parent to the edit_node anymore
		// Also, we wouldn't bother copying the links in this case
		// As the intention is to leave the carracass of the edit node behind to point to children node
	}
	//incomplete
}


bool Flash_Key_Value_Tree::add_growth_node_with_key_value(uint32_t key, uint8_t size, uint8_t* value){
	/*	- add new <Key, Value>:
					- First Make sure that the said key doesn't already exist'
					- Find the growth node
					- Create a RAM copy of the new node
					{
						- Find the starting_location_of_unwritten_flash
						- Add a link from the growth node to the new node
						(These two need to be done together because the second)
						(one depends on the first one and vice versa)
					}
					- Write the node to flash at starting_location_of_unwritten_flash */

	auto [growth_node_addr,__node_addr1] = find_growth_node();
	auto growth_node = Key_Value_Flash_Node::read_node_from_flash(growth_node_addr);
	Node_Address new_node_addr = growth_node.mark_growth_link_valid_and_return_address_it_points_to();
	Key_Value_Ram_Node new_node = Key_Value_Ram_Node::create_new_growth_node(key, size, value);
	return new_node.write_to_address(new_node_addr);
}

bool Flash_Key_Value_Tree::add_first_root_node(uint32_t key, uint8_t size, uint8_t* value){
// obviously unimplemented
}

bool Flash_Key_Value_Tree::add_edit_key_value(uint32_t key, uint8_t size, uint8_t* value){

	if (!state<Tree_State::rooted>())
		return add_first_root_node(key, size, value);

	auto [node_addr, parent_addr] = find_node_and_parent_matching_key(key);
	if(node_addr != 0)
		return update_node(key,size,value,node_addr,parent_addr);
	else
		return add_growth_node_with_key_value(key, size, value);
}

