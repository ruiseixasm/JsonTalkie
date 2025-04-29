#include "BroadcastSocket_EtherCard.h"

BroadcastSocket_EtherCard* BroadcastSocket_EtherCard::_instance = nullptr;

BroadcastSocket_EtherCard::BroadcastSocket_EtherCard(uint16_t port, uint8_t* mac, uint8_t csPin) 
  : _port(port), _mac(mac), _csPin(csPin), _remotePort(0), _recvLength(0) {
    memset(_remoteIp, 0, sizeof(_remoteIp));
    memset(_recvBuffer, 0, sizeof(_recvBuffer));
    _instance = this;
}

bool BroadcastSocket_EtherCard::begin() {
    if (ether.begin(ETHER_BUFFER_SIZE, _mac, _csPin)) {
        ether.udpServerListenOnPort(udpCallback, _port);
        return true;
    }
    return false;
}

void BroadcastSocket_EtherCard::end() {
    _instance = nullptr;
}

bool BroadcastSocket_EtherCard::write(const uint8_t* data, size_t length) {
    if (length > UDP_BUFFER_SIZE) return false;
    uint8_t broadcastIp[] = {255,255,255,255};
    ether.sendUdp((char*)data, length, _port, broadcastIp, _port);
    return true;
}

bool BroadcastSocket_EtherCard::available() {
    return _recvLength > 0;
}

size_t BroadcastSocket_EtherCard::read(uint8_t* buffer, size_t size) {
    if (!buffer || size == 0) return 0;
    size_t toCopy = min(size, _recvLength);
    memcpy(buffer, _recvBuffer, toCopy);
    _recvLength = 0;
    return toCopy;
}

void BroadcastSocket_EtherCard::handlePacket(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len) {
    if (len > UDP_BUFFER_SIZE) return;
    memcpy(_recvBuffer, data, len);
    _recvLength = len;
    memcpy(_remoteIp, src_ip, 4);
    _remotePort = src_port;
}

void BroadcastSocket_EtherCard::udpCallback(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len) {
    if (_instance) {
        _instance->handlePacket(dest_port, src_ip, src_port, data, len);
    }
}
