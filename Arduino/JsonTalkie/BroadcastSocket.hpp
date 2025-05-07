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

// No std::function, no templates, just a raw function pointer.
typedef void (*SocketCallback)(const char* data, size_t length);

class BroadcastSocket {
protected:
    static SocketCallback _socketCallback;

public:
    virtual BroadcastSocket() = default;
    virtual ~BroadcastSocket() = default;

    // Open/close (like ether's listen/close)
    virtual bool open(uint16_t port = 5005) = 0;
    virtual void close() = 0;

    // Send data (broadcast by default)
    virtual bool send(const char* data, size_t len) = 0;
    virtual void receive() = 0; // Just a trigger

    // Set callback (like ether's udpServerListenOnPort)
    static void setCallback(SocketCallback callback) { // Just a wrapper
        _socketCallback = callback;
    }

    // // Disable copying (socket resources are unique)
    // BroadcastSocket(const BroadcastSocket&) = delete;
    // BroadcastSocket& operator=(const BroadcastSocket&) = delete;
};


SocketCallback BroadcastSocket::_socketCallback = nullptr;

#endif // BROADCAST_SOCKET_HPP
