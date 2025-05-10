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
#ifndef BROADCAST_SOCKET_ESP32_HPP
#define BROADCAST_SOCKET_ESP32_HPP

#include "../BroadcastSocket.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

// #define BROADCAST_SOCKET_DEBUG
// #define ENABLE_DIRECT_ADDRESSING

class BroadcastSocket_ESP32 : public BroadcastSocket {
private:
    static bool _you_got_message;
    WiFiUDP _udp;
    static WiFiUDP* _udp_instance; // Needed for callback access

    // Handler for received packets
    static void handlePacket() {
        int packetSize = _udp_instance->parsePacket();
        if (packetSize) {
            #ifdef BROADCAST_SOCKET_DEBUG
            Serial.print(F("R: "));
            #endif

            if (packetSize < BROADCAST_SOCKET_BUFFER_SIZE - 1) {
                // Get sender's IP
                IPAddress remoteIp = _udp_instance->remoteIP();
                for (uint8_t byte_i = 0; byte_i < 4; ++byte_i) {
                    _source_ip[byte_i] = remoteIp[byte_i];
                }

                // Read data
                int len = _udp_instance->read(_buffer, BROADCAST_SOCKET_BUFFER_SIZE - 1);
                _buffer[len] = '\0';
                _you_got_message = true;

                #ifdef BROADCAST_SOCKET_DEBUG
                Serial.write(_buffer, len);
                Serial.println();
                #endif
            }
        }
    }

public:
    BroadcastSocket_ESP32(uint16_t port) {
        _port = port;
        _udp_instance = &_udp;
        _udp.begin(port);
    }

    bool send(const char* data, size_t size, bool as_reply = false) override {
        IPAddress broadcastIp(255, 255, 255, 255);

        #ifdef ENABLE_DIRECT_ADDRESSING
        if (as_reply) {
            _udp.beginPacket(IPAddress(_source_ip[0], _source_ip[1], _source_ip[2], _source_ip[3]), _port);
        } else {
            _udp.beginPacket(broadcastIp, _port);
        }
        #else
        _udp.beginPacket(broadcastIp, _port);
        #endif

        _udp.write((const uint8_t*)data, size);
        _udp.endPacket();

        #ifdef BROADCAST_SOCKET_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);
        Serial.println();
        #endif

        return true;
    }

    bool receive() override {
        handlePacket();
        if (_you_got_message) {
            _you_got_message = false;
            return true;
        }
        return false;
    }
};

bool BroadcastSocket_ESP32::_you_got_message = false;
WiFiUDP* BroadcastSocket_ESP32::_udp_instance = nullptr;
BroadcastSocket_ESP32 broadcast_socket(5005);

#endif // BROADCAST_SOCKET_ESP32_HPP
