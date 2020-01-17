#include <tuple>

template <int16_t max>
class CircularBuffer
{
	private:
	char buffer[max];
	int16_t s, q, e; //See above text for a better understanding of what these do
	// s: sending, q: queued, e: empty

	public:
	CircularBuffer():s(0),q(0),e(0){};

	int16_t length_of_empty_region();
	int16_t length_of_ongoing_transmission();
	int16_t length_of_queue();

	int16_t write_to_queue(char* a, int16_t);
	std::tuple<char*, int16_t> longest_possible_send();
	void send_complete();

};
