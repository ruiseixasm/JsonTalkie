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
#ifndef BROADCAST_SOCKET_UDP_HPP
#define BROADCAST_SOCKET_UDP_HPP

#include "../BroadcastSocket.hpp"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <Ethernet.h>
#include <EthernetUdp.h>


class BroadcastSocket_UDP : public BroadcastSocket {
private:
    EthernetUDP _udp;
    uint16_t _port;
    IPAddress _remoteIP;
    uint16_t _remotePort;
    bool _isInitialized = false;
    
public:
    explicit BroadcastSocket_UDP(uint16_t port = 5005) : _port(port) {} // Port set here
    
    bool begin() override {
        if (_isInitialized) return true;
        if (_udp.begin(_port)) { // Uses _port from constructor
            _isInitialized = true;
            return true;
        }
        return false;
    }

    void end() override {
        if (_isInitialized) {
            _udp.stop();
            _isInitialized = false;
        }
    }

    bool write(const uint8_t* data, size_t length) override {
        if (!_isInitialized) return false;
        _udp.beginPacket(IPAddress(255, 255, 255, 255), _port);
        _udp.write(data, length);
        return _udp.endPacket();
    }

    bool available() override {
        return _isInitialized && (_udp.parsePacket() > 0);
    }

    size_t read(uint8_t* buffer, size_t size) override {
        if (!_isInitialized) return 0;
        _remoteIP = _udp.remoteIP();
        _remotePort = _udp.remotePort();
        return _udp.read(buffer, size);
    }

    
    IPAddress getSenderIP() const { return _remoteIP; }
    uint16_t getSenderPort() const { return _remotePort; }

};


#endif // BROADCAST_SOCKET_UDP_HPP
