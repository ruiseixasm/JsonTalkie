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
typedef void (*SocketCallback)(uint16_t port, const uint8_t* data, size_t length);

class BroadcastSocket {
public:
    virtual ~BroadcastSocket() = default;

    // Open/close (like ether's listen/close)
    virtual bool open(uint16_t port) = 0;
    virtual void close() = 0;

    // Send data (broadcast by default)
    virtual bool send(uint16_t port, const uint8_t* data, size_t len) = 0;
    virtual size_t receive(const uint8_t* data, size_t len) = 0;

    // Set callback (like ether's udpServerListenOnPort)
    virtual void setCallback(SocketCallback callback) = 0;

    // Disable copying (socket resources are unique)
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
};

#endif // BROADCAST_SOCKET_HPP
