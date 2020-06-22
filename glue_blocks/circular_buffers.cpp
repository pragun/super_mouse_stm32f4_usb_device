/**
 * Title: Circular Buffer
 * Author: Pragun Goyal, December 31, 2019
 * Description: A simple circular buffer implementation specifically tailored to be used with DMA UART transmission
 * License:
 */

/**
 * Circular buffer data transmission scheme
 *
 * At some generic point in time:
 *
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X||X|X||X|
 * |--- already tx'ed----|--- tx'ing ----|--tx queue --- |---- already tx`ed-----|
 *  0                     s               q               e                       max
 *
 *  Loop invariants:
 *  0: start of array
 *  s: start of tx'ing, data is being sent out in the ongoing tx request
 *  q-1: end of data that is being sent out in the ongoing tx request
 *  q: Start of data waiting to be sent out
 *  e-1: End of data waiting to be sent out
 *  e: Start of writable area
 *
 *  max: size of the array (the last index is max-1)
 *
 *  array[s:q-1]: data that is tx'ing
 *  array[0:s-1, e:max]: new data can be written here
 *  array[q:e-1]: data that is in the buffer waiting to be put on the next sent request
 *
 * Lets assume that at this point, there is a printf to print "cat".
 * Its easy, it would look like this
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
 * |--- already tx'ed----|--- tx'ing ----|--tx queue --------- | already tx`ed --|
 *  0                     s               q                     e                 max
 *
 * Now let us assume that at this point the TX Complete interrupt handler comes and
 * says, hurray! we've finished sending the previous block of data, so it would look
 * like
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
 * |--- already tx'ed--------------------|--tx queue --------- | already tx`ed --|
 *  0                     s               q                     e                 max
 *
 * For just a bit, and then we can dispatch another tx request from `q to `e, and
 * it would look like
 * |X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|X|X|X|X|X||X|X||X|
 * |--- already tx'ed--------------------|--tx'ing ----------- ||already tx`ed --|
 *  0                                     s                     q,e               max
 *
 * Note, that the `q and `e indices are exactly the same, because there is no
 * data waiting in the tx-queue.
 * So, now, lets assume that at this point there is a printf for "Hello World!!!".
 * Which is a longer string than we can hold in the tailing part of the buffer
 *
 * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
 * |tx queue-|--- already tx'ed----------|--tx'ing ----------- |---- tx queue ---|
 *  0         e                           s                     q                 max
 *
 * So then how would we send out the next batch of data?
 * Well, we'd know that when `e < `q, then it must have wrapped itself over
 * we can send the data from `q:`max safely as it is all increasing indices for the
 * TX function. So, whenever we receive the next TX Complete Interrupt, we'd fire
 * off a partial TX like this
 *
 * |l|d|!|!|!|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|X|c|a|t|H|e|l|l|o| |W|o|r|
 * |tx queue-|--- already tx'ed------------------------------- |---- tx'ing -----|
 *  0,q     e                                                   s                 max
 *
 *
 *  For new string of size < (max - e)
 *
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
	ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite >(n);
}

template class RingBuffer<char, uint16_t, uart_buffer_size, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
template class RingBuffer<char, uint16_t, hid_buffer_size, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;

