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
#ifndef BROADCAST_SOCKET_UDP_H
#define BROADCAST_SOCKET_UDP_H

#include "BroadcastSocket.h"
#include <Ethernet.h>
#include <EthernetUdp.h>

class BroadcastSocket_UDP : public BroadcastSocket {
public:
    explicit BroadcastSocket_UDP(uint16_t port = 5005); // Port set here
    
    bool begin() override; // No parameters
    void end() override;
    bool write(const uint8_t* data, size_t length) override;
    bool available() override;
    size_t read(uint8_t* buffer, size_t size) override;
    
    IPAddress getSenderIP() const { return _remoteIP; }
    uint16_t getSenderPort() const { return _remotePort; }

private:
    EthernetUDP _udp;
    uint16_t _port;
    IPAddress _remoteIP;
    uint16_t _remotePort;
    bool _isInitialized = false;
};

#endif // BROADCAST_SOCKET_UDP_H
