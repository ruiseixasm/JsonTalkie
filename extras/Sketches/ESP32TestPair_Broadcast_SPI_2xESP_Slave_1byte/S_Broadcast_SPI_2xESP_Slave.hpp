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


// #define BROADCAST_SPI_DEBUG
// #define BROADCAST_SPI_DEBUG_TIMING


class S_Broadcast_SPI_2xESP_Slave : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "Broadcast_SPI_2xESP_Slave"; }


	#ifdef BROADCAST_SPI_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

protected:

	enum SpiState {
		WAIT_STATUS,
		TX_PAYLOAD,
		RX_PAYLOAD
	};

	bool _initiated = false;
	const spi_host_device_t _host;

	// Two TX buffers are required because the SPI slave driver uses DMA and does not copy TX data.
	// DMA descriptors are built from the tx_buffer pointer at queue time and may be reused.
	// Alternating the buffer address guarantees a new DMA descriptor and prevents the previous
	// payload from being transmitted again.
	uint8_t _tx_payload_buffer[2][TALKIE_BUFFER_SIZE] __attribute__((aligned(4))) = {0};
	uint8_t _rx_payload_buffer[2][TALKIE_BUFFER_SIZE] __attribute__((aligned(4))) = {0};
	// In order for each one be aligned at 4, each one shall have 4 bytes long,
	// despite whe only use the first byte of each
	uint8_t _tx_status_byte[2][4] __attribute__((aligned(4))) = {0};
	uint8_t _rx_status_byte[2][4] __attribute__((aligned(4))) = {0};	// <-- aligned
	uint8_t _tx_index = 0;
	uint8_t _rx_index = 0;

	SpiState _spi_state = WAIT_STATUS;
	spi_slave_transaction_t _data_trans __attribute__((aligned(4)));
	size_t _payload_length = 0;
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

			const size_t tx_length = (size_t)_tx_status_byte[_tx_index][0];
			const size_t rx_length = (size_t)_rx_status_byte[_rx_index][0];


			/* === SPI "ISR" === */

			switch (_spi_state) {

				case WAIT_STATUS:
				{
					if (rx_length == 0) {	// It's a beacon

						if (tx_length > 0) {
							queue_tx(tx_length);
							return;
						}

					} else if (rx_length < TALKIE_BUFFER_SIZE + 1) {

						if (rx_length > 0) {
							queue_rx(rx_length);
							return;
						}
					}
				}
				break;
				
				case TX_PAYLOAD:
				{
					if (tx_length > 0) {

						#ifdef BROADCAST_SPI_DEBUG
							Serial.printf("Sent %u bytes: ", tx_length);
							for (uint8_t i = 0; i < tx_length; i++) {
								char c = _tx_payload_buffer[_tx_index][i];
								if (c >= 32 && c <= 126) Serial.print(c);
								else Serial.printf("[%02X]", c);
							}
							Serial.println();
						#endif

						_tx_index ^= 1;			// Rotate TX index
						_payload_length = 0;	// payload was sent
					}
				}
				break;
				
				case RX_PAYLOAD:
				{
					
					#ifdef BROADCAST_SPI_DEBUG
						Serial.printf("Received %u bytes: ", rx_length);
						for (uint8_t i = 0; i < rx_length; i++) {
							char c = _rx_payload_buffer[_tx_index][i];
							if (c >= 32 && c <= 126) Serial.print(c);
							else Serial.printf("[%02X]", c);
						}
						Serial.println();
					#endif

					if (_stacked_transmissions < 3) {
						
						JsonMessage new_message(
							reinterpret_cast<const char*>( _rx_payload_buffer[_tx_index] ), rx_length
						);
						
						_rx_index ^= 1;			// Rotate RX index
						queue_status();	// Safe to star before given tha tit's a different buffer (just a byte)

						_stacked_transmissions++;
						_startTransmission(new_message);
						_stacked_transmissions--;

						return;	// Avoids the queue_status() call bellow (no repetition)
					}
				}
				break;
			}
			queue_status();
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
			// Both, _tx_buffer and _payload_length, set at the same time
			char* payload_buffer = reinterpret_cast<char*>( _tx_payload_buffer[_tx_index] );
			_payload_length = json_message.serialize_json(payload_buffer, TALKIE_BUFFER_SIZE);
			return true;
		}
        return false;
    }

	
    // Specific methods associated to ESP SPI as Slave
	
	void queue_status() {
		_spi_state = WAIT_STATUS;
		_tx_status_byte[_tx_index][0] = (uint8_t)_payload_length;

		// Full-Duplex
		spi_slave_transaction_t *t = &_data_trans;
		memset(t, 0, sizeof(_data_trans));  // clear entire struct
		t->length    = 1 * 8;
		t->tx_buffer = _tx_status_byte[_tx_index];
		t->rx_buffer = _rx_status_byte[_rx_index];

		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}


	void queue_tx(size_t length = 0) {
		_spi_state = TX_PAYLOAD;
		// Half-Duplex
		spi_slave_transaction_t *t = &_data_trans;
		memset(t, 0, sizeof(_data_trans));  // clear entire struct
		t->length    = length * 8;
		t->tx_buffer = _tx_payload_buffer[_tx_index];
		t->rx_buffer = nullptr;
		
		spi_slave_queue_trans(_host, t, portMAX_DELAY);
	}


	void queue_rx(size_t length = 0) {
		_spi_state = RX_PAYLOAD;
		// Half-Duplex
		spi_slave_transaction_t *t = &_data_trans;
		memset(t, 0, sizeof(_data_trans));  // clear entire struct
		t->length    = length * 8;
		t->tx_buffer = nullptr;
		t->rx_buffer = _rx_payload_buffer[_tx_index];
		
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
			queue_status();
			_initiated = true;
		}

		#ifdef BROADCAST_SPI_DEBUG
			Serial.println("Slave ready");
		#endif
    }

};


#endif // BROADCAST_SPI_ESP2X_SLAVE_HPP
