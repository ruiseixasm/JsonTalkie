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
#define ETHER_BUFFER_SIZE 256   // Typical size for ENC28J60 is 500, but for UDP only 256 is good enough

class BroadcastSocket_EtherCard : public BroadcastSocket {
private:

    static BroadcastSocket_EtherCard* _instance;
    uint8_t* _mac;
    uint8_t* _myIp;
    uint8_t* _gwIp;
    uint8_t* _dnsIp;
    uint8_t* _mask;
    uint8_t _csPin;
    uint16_t _port;
    bool _dhcp = false;
    bool _isOpen = false;


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
    // Arduino default SPI pins in https://docs.arduino.cc/language-reference/en/functions/communication/SPI/
    BroadcastSocket_EtherCard(
        uint8_t* mac,
        const uint8_t* my_ip,
        const uint8_t* gw_ip = 0,
        const uint8_t* dns_ip = 0,
        const uint8_t* mask = 0,
        uint8_t csPin = CS) // CS is the pin 10 in Arduino boards
        : _mac(mac), _myIp(my_ip), _gwIp(gw_ip), _dnsIp(dns_ip), _mask(mask), _csPin(csPin) {}
    
    BroadcastSocket_EtherCard(
        uint8_t* mac,
        uint8_t csPin = CS) // CS is the pin 10 in Arduino boards
        : _mac(mac), _myIp(0), _gwIp(0), _dnsIp(0), _mask(0), _csPin(csPin), _dhcp(true) {}


    bool open(uint16_t port = 5005) override {
        if (_isOpen) {
            Serial.println("Already open");
            return true;
        }
        // ether is a global instantiation
        if (!ether.begin(ETHER_BUFFER_SIZE, _mac, _csPin)) {
            Serial.println("Failed to access ENC28J60");
            return false;
        }
        if (_dhcp) {
            // DHCP mode (just wait for an IP, timeout after 10 seconds)
            uint32_t start = millis();
            while (!ether.dhcpSetup() && (millis() - start < 10000)) {
                delay(100);  // Short delay between retries
            }
            if (!ether.dhcpSetup()) {
                Serial.println("Failed to get a dynamic IP");
                return false;
            }
        } else {
            // Static IP mode
            if (!ether.staticSetup(_myIp, _gwIp, _dnsIp, _mask)) {
                Serial.println("Failed to set static IP");
                return false;
            }
        }
        ether.enableBroadcast();
        _port = port;
        _isOpen = true;
        return true;
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

    bool available() override {
        // Process incoming network traffic
        _recvLength = ether.packetReceive();
        
        // If we received a packet, process it
        if (_recvLength > 0) {
            ether.packetLoop(_recvLength);  // Handle network maintenance
            
            // Check if UDP server is actively listening
            if (ether.udpServerListening()) {
                return true;  // Valid UDP packet available
            }
        }
        
        return false;  // No data available
    }

    // bool available() override {
    //     // Check if a new UDP packet is waiting
    //     _recvLength = ether.packetReceive();  // Check for incoming packets
    //     ether.packetLoop(_recvLength);        // Process network maintenance
        
    //     // Return true if we have data (UDP payload length > 0)
    //     return (_recvLength > 0);
    // }


    // bool available() override {
    //     // wait for an incoming UDP packet, but ignore its contents
    //     if (ether.packetLoop(ether.packetReceive())) {
    //         return true;
    //         // memcpy_P(ether.tcpOffset(), page, sizeof page);
    //         // ether.httpServerReply(sizeof page - 1);
    //     }
    //     return _recvLength > 0;
    // }

    // bool available() override {
    //     // // Force process all waiting packets
    //     // while (true) {
    //     //     uint16_t len = ether.packetReceive();
    //     //     if (len == 0) break;
    //     //     ether.packetLoop(len);
    //     // }
    //     return _recvLength > 0;
    // }


    size_t read(uint8_t* buffer, size_t size) override {
        Serial.println("Reading...");
        if (size == 0 || _recvLength == 0) return 0;
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
