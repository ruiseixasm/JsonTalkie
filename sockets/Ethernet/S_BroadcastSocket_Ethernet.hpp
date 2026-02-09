/**
 * @file    S_BroadcastSocket_Ethernet.hpp
 * @author  Rui Seixas Monteiro
 * @brief   A Broadcast Socket for W5500 or W5100 shields normally associated with Arduino Mega, more memory required.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - A W5500 or W5100 shield coupled with an Arduino Mega or similar configuration
 * 
 * Created: 2026-02-09
 */

#ifndef BROADCAST_SOCKET_ETHERNET_HPP
#define BROADCAST_SOCKET_ETHERNET_HPP

#include <BroadcastSocket.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// #define BROADCAST_ETHERNET_DEBUG
#define ENABLE_DIRECT_ADDRESSING


class S_BroadcastSocket_Ethernet : public BroadcastSocket {
public:
	
	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "BroadcastSocket_Ethernet"; }

protected:

	IPAddress _my_ip;
    uint16_t _port = 5005;
    EthernetUDP* _udp = nullptr;
	
	struct FromTalker {
		char name[TALKIE_NAME_LEN] = {'\0'};
		IPAddress ip_address;
	};
	FromTalker _from_talker;


    // Constructor
    S_BroadcastSocket_Ethernet() : BroadcastSocket() {}


    void _receive() override {
        if (_udp) {

			if (Ethernet.localIP() != _my_ip) {
				_my_ip = Ethernet.localIP();
				JsonMessage noise_message;
				// No need for broadcast setting, it's for same LAN Sockets only
				noise_message.set_message_value(MessageValue::TALKIE_MSG_NOISE);
				// Sends noise to everybody else. Noise will trigger a reset of the kept ips on other sockets
				_finishTransmission(noise_message);
			}

			// Receive packets
			int packetSize = _udp->parsePacket();
			if (packetSize > 0) {

        		// ===== [SELF IP] By design it doesn-t receive from SELF =====

				#ifdef BROADCAST_ETHERNET_DEBUG
				Serial.println(F("\treceive1: Packet NOT sent from this socket"));
				Serial.print(F("\t\tRemote IP: "));
				Serial.println(_udp->remoteIP());
				#endif

				JsonMessage new_message;
				char* message_buffer = new_message._write_buffer((size_t)packetSize);
				if (!message_buffer) return;	// Avoids overflow

				int length = _udp->read(message_buffer, static_cast<size_t>(packetSize));
				if (length == packetSize) {
				
					new_message._set_length(length);
					_startTransmission(new_message);
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
			if (json_message.is_to_name(_from_talker.name)) {
				if (!_udp->beginPacket(_from_talker.ip_address, _port)) {
					#ifdef BROADCAST_ETHERNET_DEBUG
					Serial.println(F("Failed to begin packet"));
					#endif
					return false;
				}
			} else if (!_udp->beginPacket(broadcastIP, _port)) {
				#ifdef BROADCAST_ETHERNET_DEBUG
				Serial.println(F("Failed to begin packet"));
				#endif
				return false;
			}
			#else
			if (!_udp->beginPacket(broadcastIP, _port)) {
				#ifdef BROADCAST_ETHERNET_DEBUG
				Serial.println(F("Failed to begin packet"));
				#endif
				return false;
			}
			#endif

			size_t bytesSent = _udp->write(
				reinterpret_cast<const uint8_t*>( json_message._read_buffer() ),
				json_message.get_length()
			);
			(void)bytesSent; // Silence unused variable warning

			if (!_udp->endPacket()) {
				#ifdef BROADCAST_ETHERNET_DEBUG
				Serial.println(F("Failed to end packet"));
				#endif
				return false;
			}

			#ifdef BROADCAST_ETHERNET_DEBUG
			Serial.print(F("S: "));
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
    static S_BroadcastSocket_Ethernet& instance() {
        static S_BroadcastSocket_Ethernet instance;
        return instance;
    }


    void set_port(uint16_t port) { _port = port; }
    void set_udp(EthernetUDP* udp) {
		
		_udp = udp;
	}
};

#endif // BROADCAST_SOCKET_ETHERNET_HPP
