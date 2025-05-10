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

// No std::function, no templates, just a raw function pointer.
typedef void (*SocketCallback)(const char* data, size_t length, const uint8_t* source_ip = 0);

class BroadcastSocket {
protected:
    static uint16_t BroadcastSocket_EtherCard::_port;
    static char* _buffer[BROADCAST_SOCKET_BUFFER_SIZE];

public:
    virtual ~BroadcastSocket() = default;

    char* get_buffer() {
        return _buffer;
    }

    // Send data (broadcast by default)
    virtual bool send(const char* data, size_t len, const uint8_t* source_ip = 0) = 0;
    virtual void receive() = 0; // Just a trigger
};

uint16_t BroadcastSocket_EtherCard::_port = 5005;
char* _buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

#endif // BROADCAST_SOCKET_HPP
