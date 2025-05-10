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


// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128

class BroadcastSocket {
protected:
    static uint8_t _source_ip[4];
    static uint16_t _port;
    static char _buffer[BROADCAST_SOCKET_BUFFER_SIZE];

public:
    virtual BroadcastSocket(uint16_t port) {
        _port = port;
    };
    virtual ~BroadcastSocket() = default;

    char* get_buffer() {
        return _buffer;
    }

    // Send data (broadcast by default)
    virtual bool send(const char* data, size_t len, bool as_reply = false) = 0;
    virtual bool receive() = 0;
};

uint8_t BroadcastSocket_EtherCard::_source_ip[4] = {0};
uint16_t BroadcastSocket_EtherCard::_port = 5005;
char BroadcastSocket_EtherCard::_buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

#endif // BROADCAST_SOCKET_HPP
