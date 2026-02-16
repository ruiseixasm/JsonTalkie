/**
 * @file    S_BroadcastSocket_EtherCard.h
 * @author  Rui Seixas Monteiro
 * @brief   A Broadcast Socket for ENC28J60 shields that requires low memory but is unable to Unicast,
 * 			ideal for Arduino Uno and Nano.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - An ENC28J60 shield coupled with an Arduino Uno or Nano
 * 
 * Created: 2026-02-09
 */

#ifndef BROADCAST_SOCKET_ETHERCARD_H
#define BROADCAST_SOCKET_ETHERCARD_H

#include <BroadcastSocket.h>
#include <EtherCard.h>


// #define BROADCAST_ETHERCARD_DEBUG
// #define ENABLE_DIRECT_ADDRESSING


class S_BroadcastSocket_EtherCard : public BroadcastSocket {
public:
	
	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "BroadcastSocket_EtherCard"; }

private:

	static JsonMessage _json_message;
    const uint16_t _port;
    static size_t _data_length;
	
	#ifdef ENABLE_DIRECT_ADDRESSING
	// Source Talker info
    static uint8_t _source_ip[4];

	struct FromTalker {
		char name[TALKIE_NAME_LEN] = {'\0'};
		uint8_t ip_address[4];
	};
	FromTalker _from_talker;
	#endif


    static void staticCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, 
                          const char* data, uint16_t length) {

        (void)src_port;	// Silence unused parameter warning
        (void)src_ip;	// Silence unused parameter warning
        (void)dst_port;	// Silence unused parameter warning

        // ===== [SELF IP] By design it doesn-t receive from SELF =====

        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("C: "));
        Serial.write(data, length);
        Serial.print(" | ");
        Serial.println(length);
        #endif

        if (length) {
			char* message_buffer = _json_message._write_buffer(length);
			if (message_buffer) {
				_json_message._set_length(length);
				memcpy(message_buffer, data, length);
				#ifdef ENABLE_DIRECT_ADDRESSING
				memcpy(_source_ip, src_ip, 4);
				#endif
				_data_length = length;	// Where is marked as received (> 0)
			}
		}
    }


protected:

    // Constructor
    S_BroadcastSocket_EtherCard(uint16_t port) : BroadcastSocket(), _port(port) {
		
		ether.udpServerListenOnPort(staticCallback, port);
	}


    void _receive() override {

		_data_length = 0;	// Makes sure this is only called once per message received (it's the Ethernet reading that sets it)
		ether.packetLoop(ether.packetReceive());	// Updates the _data_length variable
		if (_data_length) {
			
			#ifdef BROADCAST_ETHERCARD_DEBUG
			Serial.print(F("R: "));
			Serial.write(_json_message._read_buffer(), _json_message.get_length());
			Serial.println();
			#endif

			_startTransmission(_json_message);
		}
    }


	#ifdef ENABLE_DIRECT_ADDRESSING
	void _showMessage(const JsonMessage& json_message) override {

		if (json_message.has_from()) {
			json_message.get_from_name(_from_talker.name);
			memcpy(_from_talker.ip_address, _source_ip, 4);
		} else if (json_message.is_noise()) {	// Reset name keeping
			_from_talker.name[0] = '\0';	// Resets the from talker data
			return;	// It came from a Socket, no need to lose more time
		}
	}
	#endif


    bool _send(const JsonMessage& json_message) override {
        
		const char* message_buffer = json_message._read_buffer();
		size_t message_length = json_message.get_length();
		uint8_t broadcastIp[4] = {255, 255, 255, 255};
		
		#ifdef BROADCAST_ETHERCARD_DEBUG
		Serial.print(F("S: "));
		Serial.write(message_buffer, message_length);
		Serial.println();
		#endif

		#ifdef ENABLE_DIRECT_ADDRESSING
		if (json_message.is_to_name(_from_talker.name)) {
			ether.sendUdp(message_buffer, message_length, _port, _from_talker.ip_address, _port);
		} else {
			ether.sendUdp(message_buffer, message_length, _port, broadcastIp, _port);
		}
		#else
		ether.sendUdp(message_buffer, message_length, _port, broadcastIp, _port);
		#endif

		return true;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static S_BroadcastSocket_EtherCard& instance(uint16_t port = 5005) {
        static S_BroadcastSocket_EtherCard instance(port);
        return instance;
    }
	
};


#endif // BROADCAST_SOCKET_ETHERCARD_H
