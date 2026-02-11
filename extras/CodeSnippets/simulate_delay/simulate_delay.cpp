#include <iostream>

int main() {

	uint8_t _max_delay_ms = 5;
	uint16_t _last_message_timestamp = 100;

	for (uint16_t message_timestamp = 105; message_timestamp > 90; --message_timestamp) {
		const uint16_t remote_delay = _last_message_timestamp - message_timestamp;  // Package received after
		if (remote_delay >= _max_delay_ms && remote_delay < 0xFFFF / 2) {
			
			std::cout << "Message timestamp: " << message_timestamp << std::endl;
		}
	}

    return 0;
}
