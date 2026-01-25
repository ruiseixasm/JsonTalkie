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
#ifndef BROADCAST_ESP_WIFI_HPP
#define BROADCAST_ESP_WIFI_HPP


#include <BroadcastSocket.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>


// #define BROADCAST_ESP_WIFI_DEBUG
// #define BROADCAST_ESP_WIFI_DEBUG_NEW

#define ENABLE_DIRECT_ADDRESSING


class S_BroadcastESP_WiFi : public BroadcastSocket {
protected:

	IPAddress _my_ip;
	uint16_t _port = 5005;
	WiFiUDP* _udp;

	struct FromTalker {
		char name[TALKIE_NAME_LEN] = {'\0'};
		IPAddress ip_address;
	};
	FromTalker _from_talker;


    // Constructor
    S_BroadcastESP_WiFi() : BroadcastSocket() {}


    void _receive() override {

        if (_udp) {

			if (WiFi.localIP() != _my_ip) {
				_my_ip = WiFi.localIP();
				JsonMessage noise_message;
				// No need for broadcast setting, it's for same LAN Sockets only
				noise_message.set_message_value(MessageValue::TALKIE_MSG_NOISE);
				// Sends noise to everybody else. Noise will trigger a reset of the kept ips on other sockets
				_finishTransmission(noise_message);
			}

			// Receive packets
			int packetSize = _udp->parsePacket();
			if (packetSize > 0) {
				
				// ===== [SELF IP] DROP self-sent packets ===== | WiFi
				if (_udp->remoteIP() == _my_ip) {
					
					#if defined(ESP8266)
					_udp->flush();   // discard payload
					#else
					_udp->clear();   // discard payload
					#endif

					#ifdef BROADCAST_ESP_WIFI_DEBUG
					Serial.println(F("\treceive1: Dropped packet for being sent from this socket"));
					Serial.print(F("\t\tRemote IP: "));
					Serial.println(_udp->remoteIP());
					Serial.print(F("\t\tLocal IP:  "));
					Serial.println(_my_ip);
					#endif
					
					return;
				} else {
					
					#ifdef BROADCAST_ESP_WIFI_DEBUG
					Serial.println(F("\treceive1: Packet NOT sent from this socket"));
					Serial.print(F("\t\tRemote IP: "));
					Serial.println(_udp->remoteIP());
					Serial.print(F("\t\tLocal IP:  "));
					Serial.println(_my_ip);
					#endif
					
				}

				JsonMessage new_message;
				char* message_buffer = new_message._write_buffer((size_t)packetSize);
				if (!message_buffer) return;	// Avoids overflow

				int length = _udp->read(message_buffer, static_cast<size_t>(packetSize));
				if (length == packetSize) {
					
					new_message._set_length(length);
					_startTransmission(new_message);
				} else {
					
					#ifdef BROADCAST_ESP_WIFI_DEBUG
					Serial.println(F("\treceive1: Packet size miss match!"));
					Serial.print(F("\t\tlength: "));
					Serial.println(length);
					Serial.print(F("\t\tpacketSize:  "));
					Serial.println(packetSize);
					#endif
					
				}
			}
		}
    }


	void _showMessage(const JsonMessage& json_message) override {

		if (json_message.has_from()) {
			json_message.get_from_name(_from_talker.name);
			_from_talker.ip_address = _udp->remoteIP();
		} else if (json_message.is_noise()) {	// Reset name keeping
			_from_talker.name[0] = '\0';	// Resets the from talker data
			return;	// It came from a Socket, no need to lose more time
		}
	}


    bool _send(const JsonMessage& json_message) override {
		
        if (_udp) {
			
            IPAddress broadcastIP(255, 255, 255, 255);

            #ifdef ENABLE_DIRECT_ADDRESSING

			bool as_reply = json_message.is_to_name(_from_talker.name);

			#ifdef BROADCAST_ESP_WIFI_DEBUG_NEW
			Serial.print(F("\t\t\t\t\tsend orgn: "));
			json_message.write_to(Serial);
			Serial.println();
			Serial.print(F("\t\t\t\t\tsend json: "));
			json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println(as_reply);
			Serial.print(F("\t\t\t\t\tsend buff: "));
			Serial.write(
				json_message._read_buffer(),
				json_message.get_length()
			);
			Serial.print(" | ");
			Serial.println(json_message.get_length());
			#endif

            if (!_udp->beginPacket(as_reply ? _from_talker.ip_address : broadcastIP, _port)) {
                #ifdef BROADCAST_ESP_WIFI_DEBUG
                Serial.println(F("\tFailed to begin packet"));
                #endif
                return false;
            } else {
				
				#ifdef BROADCAST_ESP_WIFI_DEBUG
				if (as_reply && _from_talker.ip_address != broadcastIP) {

					Serial.print(F("\tsend1: --> Directly sent to the  "));
					Serial.print(_from_talker.ip_address);
					Serial.print(F(" address --> "));
					
				} else {
					
					Serial.print(F("\tsend1: --> Broadcast sent to the 255.255.255.255 address --> "));
					
				}
				#endif
			}
            #else
            if (!_udp->beginPacket(broadcastIP, _port)) {
                #ifdef BROADCAST_ESP_WIFI_DEBUG
                Serial.println(F("\tFailed to begin packet"));
                #endif
                return false;
            } else {
									
				#ifdef BROADCAST_ESP_WIFI_DEBUG
				Serial.print(F("\tsend1: --> Broadcast sent to the 255.255.255.255 address --> "));
				#endif

			}
            #endif

            size_t bytesSent = _udp->write(
				reinterpret_cast<const uint8_t*>( json_message._read_buffer() ),
				json_message.get_length()
			);
            (void)bytesSent; // Silence unused variable warning

            if (!_udp->endPacket()) {
                #ifdef BROADCAST_ESP_WIFI_DEBUG
                Serial.println(F("\n\t\tERROR: Failed to end packet"));
                #endif
                return false;
            }

            #ifdef BROADCAST_ESP_WIFI_DEBUG
            Serial.write(
				json_message._read_buffer(),
				json_message.get_length()
			);
            Serial.println();
            #endif

			return true;
        }
        return false;
    }


public:

    // Move ONLY the singleton instance method to subclass
    static S_BroadcastESP_WiFi& instance() {
        static S_BroadcastESP_WiFi instance;
        return instance;
    }

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "BroadcastESP_WiFi"; }


    void set_port(uint16_t port) { _port = port; }
    void set_udp(WiFiUDP* udp) {
        
        // ===== [SELF IP] store local IP for self-filtering =====
        _my_ip = WiFi.localIP();
        _udp = udp;
    }

};

#endif // BROADCAST_ESP_WIFI_HPP
