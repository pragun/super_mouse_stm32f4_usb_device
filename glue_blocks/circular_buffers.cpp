/**
 * Title: Circular Buffer
 * Author: Pragun Goyal, December 31, 2019
 * Description: A simple circular buffer implementation specifically tailored to be used with DMA UART transmission
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

uint16_t UART_Tx_CircularBuffer::length_of_empty_region(){
	return ring_buf.get_num_elements<RingState::ClearToWrite>();
}

uint16_t UART_Tx_CircularBuffer::length_of_ongoing_transmission(){
	return ring_buf.get_num_elements<RingState::Sending>();
}

uint16_t UART_Tx_CircularBuffer::length_of_queue(){
	return ring_buf.get_num_elements<RingState::Queued>();
}

/* --- */
void HIDContinuousBlockCircularBuffer::copy_in_without_rollover(uint16_t n, char* txt) {
	ring_buf.copy_in_without_rollover<RingState::ClearToWrite, RingState::Queued>(n, txt);
}

void HIDContinuousBlockCircularBuffer::print_state() {
	ring_buf.print_state();
}

char* HIDContinuousBlockCircularBuffer::transfer_out(uint16_t n) {
	return ring_buf.get_transfer_buffer<RingState::Queued, RingState::Sending, false>(n);
}

void HIDContinuousBlockCircularBuffer::mark_transferred(uint16_t n) {
	ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite>(n);
}

template class RingBuffer<char, uint16_t, uart_buffer_size, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
template class RingBuffer<char, uint16_t, hid_buffer_size, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;

