#include "ring_buffer.cpp"
#include "ring_buffer_instances.hpp"

void TrimmedRingBufferforHID::copy_in_without_rollover(uint16_t n, char* txt) {
	ring_buf.copy_in_without_rollover<RingState::ClearToWrite, RingState::Queued>(n, txt);
}

void TrimmedRingBufferforHID::print_state() {
	ring_buf.print_state();
}

char* TrimmedRingBufferforHID::transfer_out(uint16_t n) {
	return ring_buf.get_transfer_buffer<RingState::Queued, RingState::Sending, false>(n);
}

void TrimmedRingBufferforHID::mark_transferred(uint16_t n) {
	ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite >(n);
}


void RingBufferforUART::copy_in_with_rollover(uint16_t n, char* txt) {
	ring_buf.copy_in_with_rollover<RingState::ClearToWrite, RingState::Queued>(n, txt);
}

void RingBufferforUART::print_state() {
	ring_buf.print_state();
}

std::tuple<char*, uint16_t> RingBufferforUART::transfer_out_next_chunk() {
	return ring_buf.get_next_continuous_transfer_buffer<RingState::Queued, RingState::Sending>();
}

void RingBufferforUART::mark_transferred(uint16_t n) {
	ring_buf.mark_transferred< RingState::Sending, RingState::ClearToWrite >(n);
}


template class RingBuffer<char, uint16_t, 16, false, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
template class RingBuffer<char, uint16_t, 16, true, RingState, RingState::ClearToWrite, RingState::Queued, RingState::Sending>;
