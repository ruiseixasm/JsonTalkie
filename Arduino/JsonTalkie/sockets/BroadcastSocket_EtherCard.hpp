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



// #define BROADCAST_ETHERCARD_DEBUG
// #define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_EtherCard : public BroadcastSocket {
private:
    static size_t _data_length;
    
    // Static callback remains unchanged
    static void udpCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, 
                          const char* data, uint16_t length) {
        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("R: "));
        Serial.write(data, length);
        Serial.println();
        #endif

        _data_length = 0;
        if (dst_port == _port && length < _size - 1) {
            memcpy(_source_ip, src_ip, 4);
            memcpy(_buffer, data, length);
            _buffer[length] = '\0';
            _data_length = length;
        }
    }

    // Private constructor for singleton
    BroadcastSocket_EtherCard() {
        ether.udpServerListenOnPort(udpCallback, _port);
    }

public:
    // Singleton accessor
    static BroadcastSocket_EtherCard& instance() {
        static BroadcastSocket_EtherCard instance;
        return instance;
    }

    

    bool send(const char* data, size_t size, bool as_reply = false) override {
        uint8_t broadcastIp[4] = {255, 255, 255, 255};
        
        #ifdef ENABLE_DIRECT_ADDRESSING
        ether.sendUdp(data, size, _port, 
                     as_reply ? _source_ip : broadcastIp, _port);
        #else
        ether.sendUdp(data, size, _port, broadcastIp, _port);
        #endif

        #ifdef BROADCAST_ETHERCARD_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);
        Serial.println();
        #endif

        return true;
    }


    size_t receive(char* buffer, size_t size) override {
        initialize_buffer(buffer, size);
        ether.packetLoop(ether.packetReceive());
        return _data_length;
    }


    // Modified methods to work with singleton
    void set_port(uint16_t port) override {
        _port = port;
        ether.udpServerListenOnPort(udpCallback, _port);
    }
};

size_t BroadcastSocket_EtherCard::_data_length = 0;

#endif // BROADCAST_SOCKET_ETHERCARD_HPP
