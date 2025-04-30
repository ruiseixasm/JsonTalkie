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
#include "BroadcastSocket_UDP.h"

BroadcastSocket_UDP::BroadcastSocket_UDP(uint16_t port) : _port(port) {}

bool BroadcastSocket_UDP::begin() { // No parameters now
    if (_isInitialized) return true;
    if (_udp.begin(_port)) { // Uses _port from constructor
        _isInitialized = true;
        return true;
    }
    return false;
}

void BroadcastSocket_UDP::end() {
    if (_isInitialized) {
        _udp.stop();
        _isInitialized = false;
    }
}

bool BroadcastSocket_UDP::write(const uint8_t* data, size_t length) {
    if (!_isInitialized) return false;
    _udp.beginPacket(IPAddress(255, 255, 255, 255), _port);
    _udp.write(data, length);
    return _udp.endPacket();
}

bool BroadcastSocket_UDP::available() {
    return _isInitialized && (_udp.parsePacket() > 0);
}

size_t BroadcastSocket_UDP::read(uint8_t* buffer, size_t size) {
    if (!_isInitialized) return 0;
    _remoteIP = _udp.remoteIP();
    _remotePort = _udp.remotePort();
    return _udp.read(buffer, size);
}
