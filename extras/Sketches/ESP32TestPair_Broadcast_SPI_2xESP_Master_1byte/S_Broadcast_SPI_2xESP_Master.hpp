/*
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
*/
#ifndef BROADCAST_SPI_ESP2X_MASTER_HPP
#define BROADCAST_SPI_ESP2X_MASTER_HPP


#include <BroadcastSocket.h>
#include "driver/spi_master.h"


// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_TIMING

// Broadcast SPI is fire and forget, so, it is needed to give some time to the Slaves catch up with the next send from the Master
#define broadcast_time_slot_us 500	// Gives some time to all Slaves to process the received broadcast before a next one
#define beacon_time_slot_us 100		// Avoids too frequent beacons (used to collect data from the SPI Slaves)


class S_Broadcast_SPI_2xESP_Master : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_2xESP_Master"; }


	#ifdef BROADCAST_SPI_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

protected:

	bool _initiated = false;
	
    const int* _spi_cs_pins;
    const uint8_t _ss_pins_count;
	const spi_host_device_t _host;
	
	spi_device_handle_t _spi;
	uint8_t _tx_status_byte __attribute__((aligned(4))) = 0;
	uint8_t _rx_status_byte __attribute__((aligned(4))) = 0;
	uint8_t _data_buffer[TALKIE_BUFFER_SIZE] __attribute__((aligned(4))) = {0};

	bool _in_broadcast_slot = false;
	uint32_t _broadcast_time_us = 0;
	// Too many SPI sends to the Slaves asking if there is something to send will overload them, so, a timeout is needed
	uint32_t _last_beacon_time_us = 0;


    // Constructor
    S_Broadcast_SPI_2xESP_Master(const int* ss_pins, uint8_t ss_pins_count, spi_host_device_t host)
		: BroadcastSocket(), _spi_cs_pins(ss_pins), _ss_pins_count(ss_pins_count), _host(host) {
            
		_max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
	}


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    void _receive() override {

		// Sends once per pin, avoids getting stuck in processing many pins
		static uint8_t actual_pin_index = 0;

		if (_in_broadcast_slot && micros() - _broadcast_time_us > broadcast_time_slot_us) {
			_in_broadcast_slot = false;
		}

		// Master gives priority to broadcast send, NOT to receive, so, it respects the broadcast time slot
		if (!_in_broadcast_slot && _initiated) {
			
			// Too many SPI sends to the Slaves asking if there is something to send will overload them, so, a timeout is needed
			if (micros() - _last_beacon_time_us > beacon_time_slot_us) {
				_last_beacon_time_us = micros();	// Avoid calling the beacon right away

				#ifdef BROADCAST_SPI_DEBUG_TIMING
				_reference_time = millis();
				#endif
			
				// Arms the receiving
				size_t payload_length = receivePayload(_spi_cs_pins[actual_pin_index]);
				if (payload_length > 0) {

					#ifdef BROADCAST_SPI_DEBUG
						Serial.printf("[From Beacon to pin %d] Slave: 0x%02X Beacon=1 L=%d\n",
							_spi_cs_pins[actual_pin_index], 0b10000000 | length, length);
						Serial.print("[From Slave] Received: ");
						for (int i = 0; i < length; i++) {
							Serial.print((char)_data_buffer[i]);
						}
						Serial.println();
					#endif
					
					// No receiving while a send is pending, so, no _json_message corruption is possible
					JsonMessage new_message(reinterpret_cast<const char*>( _data_buffer ), payload_length);
					_startTransmission(new_message);
				}
				actual_pin_index = (actual_pin_index + 1) % _ss_pins_count;
			}
		}
    }

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		if (_initiated) {
			
			#ifdef BROADCAST_SPI_DEBUG_TIMING
				Serial.print("\n\tsend: ");
				_reference_time = millis();
			#endif

			#ifdef BROADCAST_SPI_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - _reference_time);
			#endif

			size_t len = json_message.serialize_json(
				reinterpret_cast<char*>( _data_buffer ),
				TALKIE_BUFFER_SIZE
			);
			
			if (len > 0) {

				while (_in_broadcast_slot) {	// Avoids too many sends too close in time
					// Broadcast has priority over receiving, so, no beacons are sent during broadcast time slot!
					if (micros() - _broadcast_time_us > broadcast_time_slot_us) _in_broadcast_slot = false;
				}

				#ifdef BROADCAST_SPI_DEBUG
				Serial.print(F("\t\t\t\t\tsend1: Sent message: "));
				Serial.write(json_message._read_buffer(), json_message.get_length());
				Serial.print(F("\n\t\t\t\t\tsend2: Sent length: "));
				Serial.println(json_message.get_length());
				#endif
			
				broadcastPayload(_spi_cs_pins, _ss_pins_count, (uint8_t)len);
				_broadcast_time_us = micros();	// send time spacing applies after the sending (avoids bursting)
				_in_broadcast_slot = true;

			} else {
				return false;
			}
			
			#ifdef BROADCAST_SPI_DEBUG
				Serial.printf("\n[From Master] Slave: 0x%02X Beacon=0 L=%d\n", len, len);
				Serial.printf("[To Slave] Sent %d bytes\n", len);
				Serial.println(F("\t\t\t\t\tsend4: --> Broadcast sent to all pins -->"));
			#endif

			#ifdef BROADCAST_SPI_DEBUG_TIMING
				Serial.print(" | ");
				Serial.print(millis() - _reference_time);
			#endif

			return true;
		}
        return false;
    }

	
    // Specific methods associated to ESP SPI as Master
	
	void broadcastPayload(const int* ss_pins, uint8_t ss_pins_count, uint8_t length) {

		if (length > TALKIE_BUFFER_SIZE) return;
		_data_buffer[0] = length;
		_data_buffer[TALKIE_BUFFER_SIZE - 1] = length;
		spi_transaction_t t = {};
		t.length = TALKIE_BUFFER_SIZE * 8;	// Bytes to bits
		t.tx_buffer = _data_buffer;
		t.rx_buffer = nullptr;

		for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
			digitalWrite(ss_pins[ss_pin_i], LOW);
		}
		spi_device_transmit(_spi, &t);
		for (uint8_t ss_pin_i = 0; ss_pin_i < ss_pins_count; ss_pin_i++) {
			digitalWrite(ss_pins[ss_pin_i], HIGH);
		}
		memset(_data_buffer, 0, sizeof(_data_buffer));  // clear sent data
		// Border already included in the broadcast time slot
	}

	size_t receivePayload(int ss_pin) {
		_data_buffer[0] = 0xF0;	// 0xF0 is to receive
		_data_buffer[TALKIE_BUFFER_SIZE - 1] = _data_buffer[0];
		spi_transaction_t t = {};
		t.length = TALKIE_BUFFER_SIZE * 8;	// Bytes to bits
		t.tx_buffer = _data_buffer;
		t.rx_buffer = _data_buffer;
		
		digitalWrite(ss_pin, LOW);
		spi_device_transmit(_spi, &t);
		digitalWrite(ss_pin, HIGH);

		if (_data_buffer[0] > 0 && _data_buffer[0] == _data_buffer[TALKIE_BUFFER_SIZE - 1]) {
			size_t payload_length = (size_t)_data_buffer[0];
			_data_buffer[0] = '{';
			_data_buffer[TALKIE_BUFFER_SIZE - 1] = '}';
			return payload_length;
		}
		return 0;
	}


public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_2xESP_Master& instance(const int* ss_pins, uint8_t ss_pins_count, spi_host_device_t host = HSPI_HOST) {
        static S_Broadcast_SPI_2xESP_Master instance(ss_pins, ss_pins_count, host);

        return instance;
    }


    void begin(int mosi_io_num, int miso_io_num, int sclk_io_num) {
		
		// ================== CONFIGURE SS PINS ==================
		// CRITICAL: Configure all SS pins as outputs and set HIGH
		for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
			pinMode(_spi_cs_pins[ss_pin_i], OUTPUT);
			digitalWrite(_spi_cs_pins[ss_pin_i], HIGH);
		}

		spi_bus_config_t buscfg = {};
		buscfg.mosi_io_num = mosi_io_num;
		buscfg.miso_io_num = miso_io_num;
		buscfg.sclk_io_num = sclk_io_num;
		buscfg.quadwp_io_num = -1;
		buscfg.quadhd_io_num = -1;
		buscfg.max_transfer_sz = TALKIE_BUFFER_SIZE;
		
		// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html

		spi_device_interface_config_t devcfg = {};
		devcfg.clock_speed_hz = 4000000;  // 4 MHz - Sweet spot!
		devcfg.mode = 0;
		devcfg.queue_size = 1;		// Only one queue is needed given that the payload is just 128 bytes
		devcfg.spics_io_num = -1,  	// DISABLE hardware CS completely! (Broadcast)

		spi_bus_initialize(_host, &buscfg, SPI_DMA_CH_AUTO);
		spi_bus_add_device(_host, &devcfg, &_spi);
		
		_initiated = true;

		#ifdef BROADCAST_SPI_DEBUG
		if (_initiated) {
			Serial.print(class_description());
			Serial.println(": initiate1: Socket initiated!");

			Serial.print(F("\tinitiate2: Total SS pins connected: "));
			Serial.println(_ss_pins_count);
			Serial.print(F("\t\tinitiate3: SS pins: "));
			
			for (uint8_t ss_pin_i = 0; ss_pin_i < _ss_pins_count; ss_pin_i++) {
				Serial.print(_spi_cs_pins[ss_pin_i]);
				Serial.print(F(", "));
			}
			Serial.println();
		} else {
			Serial.println("initiate1: Socket NOT initiated!");
		}
		#endif
    }
};


#endif // BROADCAST_SPI_ESP2X_MASTER_HPP
