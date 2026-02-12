#include <iostream>

int main() {

	const uint8_t _max_delay_ms = 5;
	const uint16_t _last_message_timestamp = 100;

	for (uint16_t message_timestamp = 105; message_timestamp > 90; --message_timestamp) {
		const uint16_t remote_delay = _last_message_timestamp - message_timestamp;  // Package received after
		if (remote_delay < 0xFFFF / 2 && remote_delay >= _max_delay_ms) {
			
			std::cout << "Message timestamp: " << message_timestamp << std::endl;
		}
	}

	std::cout << std::endl;

	// Works the same
	for (uint16_t message_timestamp = 105; message_timestamp > 90; --message_timestamp) {
		if (_last_message_timestamp - message_timestamp < 0xFFFF / 2
			&& _last_message_timestamp - message_timestamp >= _max_delay_ms) {
			
			std::cout << "Message timestamp: " << message_timestamp << std::endl;
		}
	}

	// OUTPUT:
	// [Running] cd "/mnt/extra/GitHub/JsonTalkie/extras/CodeSnippets/simulate_delay/" && g++ simulate_delay.cpp -o simulate_delay && "/mnt/extra/GitHub/JsonTalkie/extras/CodeSnippets/simulate_delay/"simulate_delay
	// Message timestamp: 95
	// Message timestamp: 94
	// Message timestamp: 93
	// Message timestamp: 92
	// Message timestamp: 91

	// Message timestamp: 95
	// Message timestamp: 94
	// Message timestamp: 93
	// Message timestamp: 92
	// Message timestamp: 91

	// [Done] exited with code=0 in 1.524 seconds



    return 0;
}
