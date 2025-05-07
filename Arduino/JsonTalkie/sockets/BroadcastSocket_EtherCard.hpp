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
#define BROADCAST_SOCKET_DEBUG

class BroadcastSocket_EtherCard : public BroadcastSocket {
private:
    uint8_t* _mac;
    uint8_t* _myIp;
    uint8_t* _gwIp;
    uint8_t* _dnsIp;
    uint8_t* _mask;
    uint8_t* _broadcastIp;
    uint8_t _csPin;
    static uint16_t _port;
    bool _dhcp = false;
    bool _isOpen = false;

    // Corrected callback as a wrapper (must be static)
    static void udpCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, const char* data, uint16_t length) {
        
        #ifdef BROADCAST_SOCKET_DEBUG    
        Serial.print("R: ");
        Serial.write(data, length);    // Properly prints raw bytes as characters
        Serial.println();           // Adds newline after the printed data
        #endif

        if (_socketCallback != nullptr && dst_port == _port)
            _socketCallback(data, length);
    }

public:
    // Arduino default SPI pins in https://docs.arduino.cc/language-reference/en/functions/communication/SPI/
    BroadcastSocket_EtherCard(
        uint8_t* mac,
        const uint8_t* my_ip,
        const uint8_t* gw_ip = 0,
        const uint8_t* dns_ip = 0,
        const uint8_t* mask = 0,
        const uint8_t* broadcast_ip = 0,
        uint8_t csPin = 10) // CS is the pin 10 in Arduino boards
        : _mac(mac), _myIp(my_ip), _gwIp(gw_ip), _dnsIp(dns_ip), _mask(mask), _csPin(csPin) {}
    
    BroadcastSocket_EtherCard(
        uint8_t* mac,
        uint8_t csPin = 10) // CS is the pin 10 in Arduino boards
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
        ether.udpServerListenOnPort(udpCallback, port);
        _port = port;
        _isOpen = true;
        return true;
    }

    //--- REINITIALIZE WITH NEW SETTINGS ---//
    // Just call begin() again! No disable() needed.
    void close() override {
        _isOpen = false;
    }

    bool send(const char* data, size_t size) override {
        if (_broadcastIp == 0) {
            uint8_t broadcastIp[] = {255,255,255,255};
            ether.sendUdp(data, size, _port, broadcastIp, _port);
        } else {
            ether.sendUdp(data, size, _port, _broadcastIp, _port);
        }

        #ifdef BROADCAST_SOCKET_DEBUG
        Serial.print("S: ");
        Serial.write(data, size);   // Properly prints raw bytes as characters
        Serial.println();           // Adds newline after the printed data
        #endif

        return true;
    }

    void receive() override {    // Just a trigger
        ether.packetLoop(ether.packetReceive());
    }

};  


#endif // BROADCAST_SOCKET_ETHERCARD_HPP
