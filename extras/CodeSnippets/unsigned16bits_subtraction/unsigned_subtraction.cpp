#include <iostream>

int main() {

	uint16_t base_number = 1000;

	for (uint16_t identity_i = 0; identity_i < 0xFFFF; identity_i++) {
		uint16_t delta_value = base_number - identity_i;
		std::cout << delta_value << std::endl;
	}
    return 0;
}
