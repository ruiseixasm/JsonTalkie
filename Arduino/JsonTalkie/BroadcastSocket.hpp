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
    static uint16_t _port;

    // Private constructor
    BroadcastSocket() = default;
    virtual ~BroadcastSocket() = default;

public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

    // Pure virtual methods remain unchanged
    virtual bool send(const char* data, size_t len, bool as_reply = false) = 0;
    virtual size_t receive(char* buffer, size_t size) = 0;
    
    virtual void set_port(uint16_t port) { _port = port; }
};

// Static member initialization
uint16_t BroadcastSocket::_port = 5005; // The default port


#endif // BROADCAST_SOCKET_HPP
