/**
 * Title: Circular Buffer
 * Author: Pragun Goyal, December 31, 2019
 * Description: A simple circular buffer implementation specifically tailored 
 * to be used with memory constrained environments (like the STM32) without
 * malloc (or where using malloc is not the best idea).
 * License:
 */

#include "ring_buffer.cpp"
#include "circular_buffers.hpp"
#include <tuple>
#include <cstring>

		
uint16_t UART_Tx_CircularBuffer::write_to_queue(char* txt, uint16_t n) {
	return ring_buf.copy_in_with_rollover<RingState::ClearToWrite, RingState::Queued>(n, txt);
}

void UART_Tx_CircularBuffer::print_state() {
	ring_buf.print_state();
}

std::tuple<char*, uint16_t> UART_Tx_CircularBuffer::longest_possible_send() {
	return ring_buf.get_next_continuous_transfer_buffer<RingState::Queued, RingState::Sending>();
}

uint16_t UART_Tx_CircularBuffer::mark_transferred(uint16_t n) {
	return ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite >(n);
}

uint16_t UART_Tx_CircularBuffer::last_send_complete() {
	return ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite >(ring_buf.get_num_elements<RingState::Sending>());
}

/*
uint16_t UART_Tx_CircularBuffer::length_of_empty_region(){
	return ring_buf.get_num_elements<RingState::ClearToWrite>();
}

uint16_t UART_Tx_CircularBuffer::length_of_ongoing_transmission(){
	return ring_buf.get_num_elements<RingState::Sending>();
}

uint16_t UART_Tx_CircularBuffer::length_of_queue(){
	return ring_buf.get_num_elements<RingState::Queued>();
}
*/

/* --- */
uint16_t HIDContinuousBlockCircularBuffer::copy_in_report(uint16_t n, char* txt) {
	char* write_buf = ring_buf.get_transfer_buffer<RingState::ClearToWrite, RingState::Queued, true>(n+sizeof(n));
	if (write_buf != nullptr) {
		std::memcpy(write_buf, &n, sizeof(n));
		std::memcpy(&write_buf[sizeof(n)], txt, n);
		return n;
	}
	return 0;
}

void HIDContinuousBlockCircularBuffer::print_state() {
	ring_buf.print_state();
}

std::tuple<char*, uint16_t> HIDContinuousBlockCircularBuffer::transfer_out_next_report() {
	//first read out uint16_t sized bytes from the queue to see how many bytes
	//to read for the actual report
	char* siz = ring_buf.get_transfer_buffer<RingState::Queued, RingState::Sending, false>(sizeof(uint16_t));
	if (siz == nullptr) { 
		uint16_t zero = 0;
		return std::tie(siz , zero);
	}

	uint16_t* size_of_report = (uint16_t*)&siz[0];
	char* data = ring_buf.get_transfer_buffer<RingState::Queued, RingState::Sending, false>(*size_of_report);

	if (data == nullptr) {
		uint16_t zero = 0;
		return std::tie(data,  zero);
	}
	return std::tie(data, *size_of_report);
}

uint16_t HIDContinuousBlockCircularBuffer::mark_transferred(uint16_t n) {
	return ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite>((n+sizeof(n)));
}

uint16_t HIDContinuousBlockCircularBuffer::last_send_complete() {
	return ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite >(ring_buf.get_num_elements<RingState::Sending>());
}

template class RingBuffer<char, uint16_t, uart_buffer_size, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
template class RingBuffer<char, uint16_t, hid_buffer_size, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
