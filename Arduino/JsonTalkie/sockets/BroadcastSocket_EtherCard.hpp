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
#ifndef BROADCAST_SOCKET_ETHERCARD_HPP
#define BROADCAST_SOCKET_ETHERCARD_HPP

#include "../BroadcastSocket.hpp"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <EtherCard.h>


// #define BROADCAST_SOCKET_DEBUG
// #define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_EtherCard : public BroadcastSocket {
private:
    static uint16_t _port;
    static bool _you_got_message;

public:
    // Corrected callback as a wrapper (must be static)
    static void udpCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, const char* data, uint16_t length) {
        
        #ifdef BROADCAST_SOCKET_DEBUG
        Serial.print(F("R: "));
        Serial.write(data, length);    // Properly prints raw bytes as characters
        Serial.println();           // Adds newline after the printed data

        if (dst_port == _port) {
            Serial.println("Package port matches");
        } else {
            Serial.println("Package port does NOT match");
        }
        #endif

        if (dst_port == _port) {
            
            if (length < BROADCAST_SOCKET_BUFFER_SIZE - 1) {
                memcpy(_buffer, data, length);
                _buffer[length] = '\0';

            _you_got_message = true;
        }
    }

    void set_buffer(const char* buffer) override {
        _buffer = buffer;
    };

    bool send(const char* data, size_t size, const uint8_t* source_ip = 0) override {

        
        #ifdef ENABLE_DIRECT_ADDRESSING
        if (source_ip == 0) {
            ether.sendUdp(data, size, _port, _broadcastIp, _port);
        } else {
            ether.sendUdp(data, size, _port, source_ip, _port);
        }
        #else
        // EtherCard can't handle direct addressing correctly, so it must reply in broadcast!
        ether.sendUdp(data, size, _port, _broadcastIp, _port);
        #endif

        #ifdef BROADCAST_SOCKET_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);   // Properly prints raw bytes as characters
        Serial.println();           // Adds newline after the printed data
        #endif

        return true;
    }

    void receive() override {    // Just a trigger
        ether.packetLoop(ether.packetReceive());

        if (_you_got_message) {
            // process message
        }
    }

};  

bool BroadcastSocket_EtherCard::_you_got_message = false;
BroadcastSocket_EtherCard broadcast_socket;

#endif // BROADCAST_SOCKET_ETHERCARD_HPP
