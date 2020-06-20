/*
 * ring_buffer.cpp
 *
 *  Created on: Jun 15, 2020
 *      Author: Pragun Goyal
 *
 */

#include <cstring>
#include <tuple>
#include <algorithm>    // std::min
#include "ring_buffer.hpp"

template <typename storage_type, typename index_type, index_type max_buffer_size, bool enable_trimming, typename RingStateEnum, RingStateEnum... states>
class RingBuffer
{
	private:
	storage_type buffer[max_buffer_size];
	index_type buffer_size = max_buffer_size;

	constexpr static  std::size_t num_states = sizeof...(states);
	index_type starting_index[num_states]; // Starting index for the data in this state
	index_type num_elements[num_states]; // Size of this section ring-state

	constexpr static bool trim_enabled = enable_trimming;

	template <RingStateEnum state>
	void inflate_to_full_size(){
		static_assert(trim_enabled, "Trimming needs to be enabled to use inflate.");
		// Check if the buffer roll-over boundary falls within this ring state
		if ((get_starting_index<state>()+get_num_elements<state>()) >= buffer_size){
			set_num_elements<state>( get_num_elements<state>()+(max_buffer_size-buffer_size) );
			buffer_size = max_buffer_size;
		}
	}

	template <RingStateEnum state>
	void trim_to_start_at_buffer_zero(){
		static_assert(trim_enabled, "Trimming needs to be enabled to use trim.");
		// Check if the buffer roll-over boundary falls within this ring state
		auto [lobe1,lobe2] = get_continuous_segment_sizes<state>();
		if ((get_starting_index<state>() + get_num_elements<state>()) >= buffer_size){
			set_num_elements<state>( get_num_elements<state>() - (buffer_size - get_starting_index<state>()) );
			buffer_size = get_starting_index<state>();
		}
	}

	template <RingStateEnum state>
	inline std::tuple<index_type, index_type> get_continuous_segment_sizes(){
		index_type starting_index = get_starting_index<state>();
		index_type num_elements = get_num_elements<state>();
		if ((starting_index + num_elements) <= buffer_size){
			return std::make_tuple(num_elements,0);
		}else{
			return std::make_tuple((buffer_size - starting_index),(starting_index + num_elements - buffer_size));
		}
	}

	template <RingStateEnum state>
	inline index_type get_num_elements(){
		return num_elements[static_cast<index_type>(state)];
	}

	template <RingStateEnum state>
	inline index_type get_starting_index(){
			return starting_index[static_cast<index_type>(state)];
	}

	template <RingStateEnum state>
	inline index_type set_num_elements(index_type i){
		num_elements[static_cast<index_type>(state)] = i;
	}

	template <RingStateEnum state>
	inline index_type set_starting_index(index_type n){
			starting_index[static_cast<index_type>(state)] = n;
	}


	public:
	RingBuffer(){
		printf("I do get called\n");
		index_type i = 0;
		for(i = 0; i < num_states; i++){
			starting_index[i] = 0;
			num_elements[i] = 0;
		}
		num_elements[0] = buffer_size;
	};

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type copy_in_without_rollover(index_type num, const storage_type* input_buffer){
		static_assert(trim_enabled, "Trimming needs to be enabled to use copying in without rollover.");
		storage_type* copy_in_buffer = get_transfer_buffer<from_state, to_state>(num);
		if (copy_in_buffer == nullptr){
			std::memcpy( &copy_in_buffer, input_buffer, num);
			return num;
		}
		return 0;
	}

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type copy_in_with_rollover(index_type num, const storage_type* input_buffer){
		if (num <= get_num_elements<from_state>()){
			index_type starting_index = get_starting_index<from_state>();
			if ((starting_index + num ) <= buffer_size){
				std::memcpy( &buffer[starting_index], input_buffer, num * sizeof(storage_type) );
			}else{
				index_type copy_len1 = (buffer_size - get_starting_index<from_state>());
				std::memcpy( &buffer[starting_index], input_buffer, copy_len1 * sizeof(storage_type) );
				std::memcpy( &buffer[0], &input_buffer[copy_len1], (num - copy_len1)*sizeof(storage_type));
			}

			mark_transferred<from_state, to_state>(num);
			return num;
		}else{
			return 0;
		}
	}

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type copy_out_with_rollover(index_type num,  storage_type* output_buffer){
		if (num <= get_num_elements<from_state>()){
			if ((get_starting_index<from_state>() + num ) <= buffer_size) {
				std::memcpy(output_buffer, &buffer[starting_index], num * sizeof(storage_type) );
			}else{
				index_type copy_len1 = (buffer_size - get_starting_index<from_state>());
				std::memcpy(output_buffer, &buffer[starting_index], copy_len1 * sizeof(storage_type) );
				std::memcpy(&output_buffer[copy_len1], &buffer[0], (num - copy_len1)*sizeof(storage_type));
			}
			mark_transferred<from_state, to_state>(num);
			return num;
		}else{
			return 0;
		}
	}

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type mark_transferred(index_type num){
		if (num <= get_num_elements<from_state>()){
			set_starting_index<from_state>((get_starting_index<from_state>()+num) % buffer_size);
			set_num_elements<from_state>( get_num_elements<from_state>() - num );
			set_num_elements<to_state>( get_num_elements<to_state>() + num );

			return num;
		}else{
			return 0;
		}
	}

	inline index_type print_state(){
		printf("Buffer:\n%s \nAddress:%p\n",buffer,(void*) buffer);
		for(index_type i = 0; i < num_states; i++){
			printf("State:%d, Starting Index:%d, Size:%d\n",i,starting_index[i],num_elements[i]);
		}
	}


	template <RingStateEnum from_state, RingStateEnum to_state>
	inline std::tuple<storage_type*, index_type> get_next_continuous_transfer_buffer(){
		index_type num = std::min(get_num_elements<from_state>(), (buffer_size - get_starting_index<from_state>()));
		mark_transferred<from_state, to_state>(num);
		return std::make_tuple(&buffer[get_starting_index<from_state>()],num);
	}

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline storage_type* get_transfer_buffer(index_type num){
		if constexpr(trim_enabled){
			inflate_to_full_size<from_state>();
		}

		auto [lobe1,lobe2] = get_continuous_segment_sizes<from_state>();

		if ( num <= lobe1 ){
			mark_transferred<from_state, to_state>(num);
			return &buffer[get_starting_index<from_state>()];
		}

		if constexpr (trim_enabled){
			if (num <= lobe2 ){
				trim_to_start_at_buffer_zero<from_state>();
				mark_transferred<from_state, to_state>(num);
				return &buffer[0]; //get_starting_index<from_state> should be zero now after trimming
			}
		}
		return nullptr; //buffer doesn't fit either of the lobes
	}
};


enum class RingState {ClearToWrite, Queued, Sending};

void test2(){
	char txt[8] = "Pragun";
	printf("\n\nTesting trim enabled\n");
	RingBuffer<char, uint16_t, 15, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending> b;
	printf("B Pointer:%p\n",(void *) &b);
	b.print_state();

	printf("\nWriting to buffer..\n");
	b.copy_in_without_rollover<RingState::ClearToWrite, RingState::Queued>(6, txt);
	b.print_state();

	printf("\nWriting to buffer..\n");
	b.copy_in_without_rollover<RingState::ClearToWrite, RingState::Queued>(6, txt);
	b.print_state();

	printf("\nTransferring 8 bytes from Queued to Sending..\n",txt);
	b.get_transfer_buffer<RingState::Queued, RingState::Sending>(8);
	b.print_state();

	printf("\nTransferring 8 bytes from Sending to ClearToWrite..\n",txt);
	b.mark_transferred<RingState::Sending, RingState::ClearToWrite>(8);
	b.print_state();

	printf("\nWriting to buffer..\n");
	b.copy_in_without_rollover<RingState::ClearToWrite, RingState::Queued>(6, txt);
	b.print_state();

	printf("\nWriting to buffer..\n");
	b.copy_in_without_rollover<RingState::ClearToWrite, RingState::Queued>(6, "Goyal ");
	b.print_state();

	printf("\nTransferring 7 bytes from Queued to Sending..\n",txt);
	b.get_transfer_buffer<RingState::Queued, RingState::Sending>(7);
	b.print_state();

	printf("\nTransferring 7 bytes from Sending to ClearToWrite..\n",txt);
	b.mark_transferred<RingState::Sending, RingState::ClearToWrite>(7);
	b.print_state();
}

void test1(){
	char txt[8] = "Pragun";
	printf("Testing trim disabled\n");
	using RB1 = RingBuffer<char, uint16_t, 16, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
	RB1 a;
	printf("A Pointer:%p\n",(void *) &a);
	a.print_state();

	printf("\nWriting to buffer..\n");
	a.copy_in_with_rollover<RingState::ClearToWrite, RingState::Queued>(6, txt);
	a.print_state();

	printf("\nWriting to buffer..\n");
	a.copy_in_with_rollover<RingState::ClearToWrite, RingState::Queued>(6, txt);
	a.print_state();

	printf("\nTransferring 8 bytes from Queued to Sending..\n",txt);
	a.get_transfer_buffer<RingState::Queued, RingState::Sending>(8);
	a.print_state();

	printf("\nTransferring 8 bytes from Sending to ClearToWrite..\n",txt);
	a.mark_transferred<RingState::Sending, RingState::ClearToWrite>(8);
	a.print_state();

	printf("\nWriting to buffer..\n");
	a.copy_in_with_rollover<RingState::ClearToWrite, RingState::Queued>(6, txt);
	a.print_state();

	printf("\nWriting to buffer..\n");
	a.copy_in_with_rollover<RingState::ClearToWrite, RingState::Queued>(6, "Goyal ");
	a.print_state();

	printf("\nTransferring 7 bytes from Queued to Sending..\n",txt);
	a.get_transfer_buffer<RingState::Queued, RingState::Sending>(7);
	a.print_state();

	printf("\nTransferring 7 bytes from Sending to ClearToWrite..\n",txt);
	a.mark_transferred<RingState::Sending, RingState::ClearToWrite>(7);
	a.print_state();
}

int main(){
	test2();
	test1();
}
