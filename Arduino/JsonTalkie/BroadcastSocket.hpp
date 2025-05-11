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
#ifndef BROADCAST_SOCKET_HPP
#define BROADCAST_SOCKET_HPP


class BroadcastSocket {
protected:
    static uint8_t _source_ip[4];
    static uint16_t _port;
    static char* _buffer;
    static size_t _size;

public:
    BroadcastSocket() = default;
    ~BroadcastSocket() = default;

    virtual void set_port(uint16_t port) {
        _port = port;
    };

    // Send data (broadcast by default)
    virtual bool send(const char* data, size_t size, bool as_reply = false) = 0;
    virtual bool receive(char* data, size_t size) = 0;
};

uint8_t BroadcastSocket::_source_ip[4] = {0};
uint16_t BroadcastSocket::_port = 5005; // The default port
char* BroadcastSocket::_buffer = nullptr;
size_t BroadcastSocket::_size = 0;


#if defined(EtherCard_h)
#include "sockets/BroadcastSocket_EtherCard.hpp"
BroadcastSocket_EtherCard broadcast_socket;
#elif defined(USE_SERIAL_SOCKET)
#include "sockets/BroadcastSocket_Serial.hpp"
BroadcastSocket_Serial broadcast_socket;
#else
#include "sockets/BroadcastSocket_Dummy.hpp"
BroadcastSocket_Dummy broadcast_socket;
#warning "No Ethernet library found or USE_SERIAL_SOCKET defined - falling back to Dummy implementation"
#endif


#endif // BROADCAST_SOCKET_HPP
