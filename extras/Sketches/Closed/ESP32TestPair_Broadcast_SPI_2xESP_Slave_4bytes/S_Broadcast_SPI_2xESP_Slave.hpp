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
#ifndef BROADCAST_SPI_ESP2X_SLAVE_HPP
#define BROADCAST_SPI_ESP2X_SLAVE_HPP


#include <BroadcastSocket.h>
extern "C" {
    #include "driver/spi_slave.h"
}


#define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_TIMING


#define border_delay_us 10

class S_Broadcast_SPI_2xESP_Slave : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_2xESP_Slave"; }


	#ifdef BROADCAST_SPI_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

protected:

	enum SpiState : uint8_t {
		WAIT_CMD,
		RX_PAYLOAD,
		TX_PAYLOAD
	};

	bool _initiated = false;
	const spi_host_device_t _host;

	uint8_t _tx_buffer[TALKIE_BUFFER_SIZE] __attribute__((aligned(4)));
	uint8_t _rx_buffer[TALKIE_BUFFER_SIZE] __attribute__((aligned(4)));
	uint8_t _rx_status[4] __attribute__((aligned(4))) = {0};
	uint8_t _tx_status[4] __attribute__((aligned(4))) = {0, 0, 0, 1};

	spi_slave_transaction_t _status_trans;
	spi_slave_transaction_t _rx_trans;
	spi_slave_transaction_t _tx_trans;

	SpiState _spi_state = WAIT_CMD;
	char _payload_data[TALKIE_BUFFER_SIZE];
	uint8_t _payload_length = 0;

	uint8_t _stacked_transmissions = 0;

    // Constructor
    S_Broadcast_SPI_2xESP_Slave(spi_host_device_t host) : BroadcastSocket(), _host(host) {
            
		_max_delay_ms = 0;  // SPI is sequencial, no need to control out of order packages
	}


    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    void _receive() override {

		if (_initiated) {
			
			// Checks if the queued element was consumed and needs to be processed, otherwise, return
    		spi_slave_transaction_t *ret = nullptr;
			if (spi_slave_get_trans_result(_host, &ret, 0) != ESP_OK) {
				return;
			}

			// At this point a queued element is consumed, as to queue a new one afterwards !

			/* === SPI "ISR" === */

			const bool beacon = (bool)_rx_status[3];
			const uint8_t rx_length = _rx_status[0];

			switch (_spi_state) {

				case WAIT_CMD:
				{
					if (beacon) {
						if (rx_length > 0 && rx_length == _rx_status[1] && rx_length == _rx_status[2] && rx_length == _payload_length) {
							
							#ifdef BROADCAST_SPI_DEBUG
								Serial.printf("\n[CMD] 0x%02X beacon=%d len=%u\n", rx_length, beacon, rx_length);
							#endif

							queue_tx(_payload_length);
							
						} else {
							queue_cmd();
						}

					} else if (rx_length > 0 && rx_length == _rx_status[1] && rx_length == _rx_status[2] && rx_length <= TALKIE_BUFFER_SIZE) {

						// #ifdef BROADCAST_SPI_DEBUG
						// 	Serial.printf("\n[CMD] 0x%02X beacon=%d len=%u\n", rx_length, beacon, rx_length);
						// #endif

						queue_rx(rx_length);
						
					} else {

						#ifdef BROADCAST_SPI_DEBUG
							Serial.printf("\n[PING] 0x%02X beacon=%d len=%u\n", rx_length, beacon, rx_length);
						#endif

						// #ifdef BROADCAST_SPI_DEBUG
						// 	Serial.println("Master ping");
						// #endif

						// An empty ping to keep alive
						queue_cmd();
					}
				}
				break;
				
				case RX_PAYLOAD:
				{

					// #ifdef BROADCAST_SPI_DEBUG
					// 	Serial.printf("Received %u bytes: ", rx_length);
					// 	for (uint8_t i = 0; i < rx_length; i++) {
					// 		char c = _rx_buffer[i];
					// 		if (c >= 32 && c <= 126) Serial.print(c);
					// 		else Serial.printf("[%02X]", c);
					// 	}
					// 	Serial.println();
					// #endif

					if (_stacked_transmissions < 3) {

						JsonMessage new_message(
							reinterpret_cast<const char*>( _rx_buffer ),
							static_cast<size_t>( rx_length )
						);
						
						// Needs the queue a new command, otherwise nothing is processed again (lock)
						// Real scenario if at this moment a payload is still in the queue to be sent and now
						// has no queue to be picked up
						queue_cmd();	// After the reading above to avoid _rx_buffer corruption
						
						_stacked_transmissions++;
						_startTransmission(new_message);
						_stacked_transmissions--;
						
					} else {

						// Shouldn't process more than 5 messages at once
						queue_cmd();
					}
				}
				break;
				
				case TX_PAYLOAD:
				{
					
					#ifdef BROADCAST_SPI_DEBUG
						Serial.printf("Sent %u bytes: ", _payload_length);
						for (uint8_t i = 0; i < _payload_length; i++) {
							char c = _tx_buffer[i];
							if (c >= 32 && c <= 126) Serial.print(c);
							else Serial.printf("[%02X]", c);
						}
						Serial.println();
					#endif

					_payload_length = 0;	// payload was sent
					_tx_status[3] = 0;	// Mark tx status as new (first beacon)
					memset(_tx_buffer, 0, sizeof(_tx_buffer));  // clear entire buffer
					queue_cmd();
				}
				break;
			}
		}
    }

    
    // Socket processing is always Half-Duplex because there is just one buffer to receive and other to send
    bool _send(const JsonMessage& json_message) override {

		if (_initiated) {
			
			const uint16_t start_waiting = (uint16_t)millis();
			while (_payload_length > 0) {

				if (_stacked_transmissions < 3) {

					_receive();	// keeps processing pending messages, mainly the ones pooled to be sent
					if ((uint16_t)millis() - start_waiting > 1 * 1000) {

						#ifdef BROADCAST_SPI_DEBUG
							Serial.println(F("\t_unlockSendingBuffer: NOT available sending buffer"));
						#endif

						return false;
					}

				// If there is already 3 messages staked, then, start dropping sends
				// Receive HAS priority over send!
				} else {	// Start dropping
					return false;
				}
			}
			
			_payload_length = (uint8_t)json_message.serialize_json(_payload_data, TALKIE_BUFFER_SIZE);
			return true;
		}
        return false;
    }

	
    // Specific methods associated to ESP SPI as Slave
	
	void queue_cmd() {
		delayMicroseconds(border_delay_us);	// Needs a small delay of separation in order to the CS pins be able to cycle
		_spi_state = WAIT_CMD;
		_tx_status[0] = _payload_length;
		_tx_status[1] = _payload_length;
		_tx_status[2] = _payload_length;
		// Full-Duplex
		spi_slave_transaction_t *t = &_status_trans;
		memset(t, 0, sizeof(_status_trans));  // clear entire struct
		memset(_tx_status, 0, sizeof(_tx_status));  // clear entire status
		memset(_rx_status, 0, sizeof(_rx_status));  // clear entire status
		t->length = 4 * 8;	// Bytes to bits
		// Cast away volatile
        t->tx_buffer = _tx_status;
        t->rx_buffer = _rx_status;
		// If you see 80 on the Master side it means the Slave wasn't given the time to respond!
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}

	void queue_rx(uint8_t len) {
		delayMicroseconds(border_delay_us);	// Needs a small delay of separation in order to the CS pins be able to cycle
		_spi_state = RX_PAYLOAD;
		// Half-Duplex
		spi_slave_transaction_t *t = &_rx_trans;
		memset(t, 0, sizeof(_rx_trans));  // clear entire struct
		memset(_rx_buffer, 0, sizeof(_rx_buffer));  // clear entire buffer
		t->length    = (size_t)len * 8;
		t->tx_buffer = nullptr;
		t->rx_buffer = _rx_buffer;
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}

	void queue_tx(uint8_t len) {
		delayMicroseconds(border_delay_us);	// Needs a small delay of separation in order to the CS pins be able to cycle
		_spi_state = TX_PAYLOAD;
		_tx_status[0] = 0;
		_tx_status[1] = 0;
		_tx_status[2] = 0;
		_tx_status[3] = 1;	// Marks the transmitted byte as 1 (done)
		// Half-Duplex
		spi_slave_transaction_t *t = &_tx_trans;
		memset(t, 0, sizeof(_tx_trans));  // clear entire struct
		memcpy(_tx_buffer, _payload_data, len);  // copies the entire payload
		t->length    = (size_t)len * 8;
		t->tx_buffer = _tx_buffer;
		t->rx_buffer = nullptr;
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}


public:

    // Move ONLY the singleton instance method to subclass
    static S_Broadcast_SPI_2xESP_Slave& instance(spi_host_device_t host = HSPI_HOST) {
        static S_Broadcast_SPI_2xESP_Slave instance(host);

        return instance;
    }


    void begin(int mosi_io_num, int miso_io_num, int sclk_io_num, int spics_io_num) {

		// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html
		
		// 1. ZERO the entire structs
		spi_bus_config_t buscfg = {};
		spi_slave_interface_config_t slvcfg = {};
		
		// 2. Set fields ONE BY ONE (no designator order issues!)
		buscfg.mosi_io_num = mosi_io_num;
		buscfg.miso_io_num = miso_io_num;
		buscfg.sclk_io_num = sclk_io_num;
		buscfg.quadwp_io_num = -1;
		buscfg.quadhd_io_num = -1;
		buscfg.max_transfer_sz = TALKIE_BUFFER_SIZE;
		
		
		// Newer ESP-IDF versions have these extra fields:
		buscfg.data4_io_num = -1;
		buscfg.data5_io_num = -1; 
		buscfg.data6_io_num = -1;
		buscfg.data7_io_num = -1;
		
		slvcfg.mode = 0;
		slvcfg.spics_io_num = spics_io_num;
		slvcfg.queue_size = 1;	// It's just 128 bytes maximum, so, a queue of 1 is all it needs
		slvcfg.post_setup_cb = nullptr;
		slvcfg.post_trans_cb = nullptr;
    
	
		// DMA channel must be given if > 32 bytes
		esp_err_t err = spi_slave_initialize(_host, &buscfg, &slvcfg, 1);  // ← CHANNEL 1
		if (err == ESP_OK) {
			queue_cmd();
			_initiated = true;
		}

		#ifdef BROADCAST_SPI_DEBUG
			Serial.println("Slave ready");
		#endif
    }

};


#endif // BROADCAST_SPI_ESP2X_SLAVE_HPP
