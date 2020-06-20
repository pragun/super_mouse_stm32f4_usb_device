/*
 * ring_buffer.hpp
 *
 *  Created on: Jun 16, 2020
 *      Author: Pragun
 */

#ifndef RING_BUFFER_HPP_
#define RING_BUFFER_HPP_

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
	void inflate_to_full_size();

	template <RingStateEnum state>
	void trim_to_start_at_buffer_zero();

	template <RingStateEnum state>
	inline std::tuple<index_type, index_type> get_continuous_segment_sizes();

	template <RingStateEnum state>
	inline index_type get_num_elements();

	template <RingStateEnum state>
	inline index_type get_starting_index();

	template <RingStateEnum state>
	inline index_type set_num_elements(index_type i);

	template <RingStateEnum state>
	inline index_type set_starting_index(index_type n);

	public:
	RingBuffer();

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type copy_in_without_rollover(index_type num, const storage_type* input_buffer);

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type copy_in_with_rollover(index_type num, const storage_type* input_buffer);

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type copy_out_with_rollover(index_type num,  storage_type* output_buffer);

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline index_type mark_transferred(index_type num);

	inline index_type print_state();

	template <RingStateEnum from_state, RingStateEnum to_state>
	inline std::tuple<storage_type*, index_type> get_next_continuous_transfer_buffer();

	template <RingStateEnum from_state, RingStateEnum to_state, bool trim_enabled>
	inline storage_type* get_transfer_buffer(index_type num);

};

#endif /* RING_BUFFER_HPP_ */
