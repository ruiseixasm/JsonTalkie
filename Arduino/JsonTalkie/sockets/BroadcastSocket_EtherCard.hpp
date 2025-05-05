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
#ifndef BROADCAST_SOCKET_ETHERCARD_HPP
#define BROADCAST_SOCKET_ETHERCARD_HPP

#include "../BroadcastSocket.hpp"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <EtherCard.h>


#define UDP_BUFFER_SIZE 128
#define ETHER_BUFFER_SIZE 500  // Typical size for ENC28J60 is 500, but 256 is good enough

class BroadcastSocket_EtherCard : public BroadcastSocket {
private:

    static BroadcastSocket_EtherCard* _instance;
    uint16_t _port;
    uint8_t* _mac;
    uint8_t _csPin;
    uint16_t _remotePort;
    uint8_t _remoteIp[4];
    uint8_t _recvBuffer[UDP_BUFFER_SIZE];
    size_t _recvLength;
    
    void handlePacket(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len) {
        if (len > UDP_BUFFER_SIZE) return;
        memcpy(_recvBuffer, data, len);
        _recvLength = len;
        memcpy(_remoteIp, src_ip, 4);
        _remotePort = src_port;
    }

    static void udpCallback(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len) {
    if (_instance) {
        _instance->handlePacket(dest_port, src_ip, src_port, data, len);
    }
}
public:
    BroadcastSocket_EtherCard(uint16_t port, uint8_t* mac, uint8_t csPin = 8) 
        : _port(port), _mac(mac), _csPin(csPin), _remotePort(0), _recvLength(0) {
            memset(_remoteIp, 0, sizeof(_remoteIp));
            memset(_recvBuffer, 0, sizeof(_recvBuffer));
            _instance = this;
    }
    
    bool begin() override {
        if (ether.begin(ETHER_BUFFER_SIZE, _mac, _csPin)) {
            ether.udpServerListenOnPort(udpCallback, _port);
            return true;
        }
        return false;
    }

    void end() override {
        _instance = nullptr;
    }

    bool write(const uint8_t* data, size_t length) override {
        if (length > UDP_BUFFER_SIZE) return false;
        uint8_t broadcastIp[] = {255,255,255,255};

        ether.sendUdp((char*)data, length, _port, broadcastIp, _port);
        
        Serial.print("W: ");
        Serial.write(data, length);  // Properly prints raw bytes as characters
        Serial.println();            // Adds newline after the printed data

        return true;
    }

    // bool available() override {
    //     // Minimal packet processing
    //     if (ether.packetReceive() > 0) {
    //         ether.packetLoop(ether.packetLength());
    //     }
    //     return _recvLength > 0;
    // }

    bool available() override {
        // // Force process all waiting packets
        // while (true) {
        //     uint16_t len = ether.packetReceive();
        //     if (len == 0) break;
        //     ether.packetLoop(len);
        // }
        return _recvLength > 0;
    }

    size_t read(uint8_t* buffer, size_t size) override {
        Serial.println("Reading...");
        if (size == 0) return 0;
        size_t toCopy = min(size, _recvLength);
        memcpy(buffer, _recvBuffer, toCopy);
        _recvLength = 0;

        Serial.print("R: ");
        Serial.write(buffer, toCopy);  // Properly prints raw bytes as characters
        Serial.println();            // Adds newline after the printed data

        return toCopy;
    }
    
    const uint8_t* getSenderIP() const { return _remoteIp; }
    uint16_t getSenderPort() const { return _remotePort; }

};  


#endif // BROADCAST_SOCKET_ETHERCARD_HPP
