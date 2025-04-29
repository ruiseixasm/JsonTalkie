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
