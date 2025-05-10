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


#define ETHER_BUFFER_SIZE 256   // Typical size for ENC28J60 is 500, but for UDP only 256 is good enough
// #define BROADCAST_SOCKET_DEBUG

uint8_t Ethernet::buffer[ETHER_BUFFER_SIZE] = {0};  // Now reported! (Essential for EtherCard)


class BroadcastSocket_EtherCard : public BroadcastSocket {
private:
    static uint8_t _broadcastIp[];
    static uint16_t _port;

    // Corrected callback as a wrapper (must be static)
    static void udpCallback(uint16_t src_port, uint8_t* src_ip, uint16_t dst_port, const char* data, uint16_t length) {
        
        #ifdef BROADCAST_SOCKET_DEBUG    
        Serial.print(F("R: "));
        Serial.write(data, length);    // Properly prints raw bytes as characters
        Serial.println();           // Adds newline after the printed data

        if (_isOpen) {
            Serial.println("Socket is open");
        } else {
            Serial.println("Socket is NOT open");
        }
        if (_socketCallback != nullptr) {
            Serial.println("Socket callback function is set");
        } else {
            Serial.println("Socket callback function is NOT set");
        }
        if (dst_port == _port) {
            Serial.println("Package port matches");
        } else {
            Serial.println("Package port does NOT match");
        }
        #endif

        if (_isOpen && _socketCallback != nullptr && dst_port == _port) {
            #ifdef BROADCAST_SOCKET_DEBUG
            Serial.println(F("Calling Socket callback..."));
            #endif
            _socketCallback(data, length, src_ip);
        }
    }

public:

    // Arduino default SPI pins in https://docs.arduino.cc/language-reference/en/functions/communication/SPI/
    bool open(const uint8_t* mac, uint8_t csPin = 10, uint16_t port = 5005) {
        if (_isOpen) {
            Serial.println(F("Already open"));
            return true;
        }
        // ether is a global instantiation
        if (!ether.begin(ETHER_BUFFER_SIZE, mac, csPin)) {
            Serial.println(F("Failed to access ENC28J60"));
            return false;
        }
        if (!ether.dhcpSetup()) {
            Serial.println(F("Failed to get a dynamic IP"));
            return false;
        }
        ether.enableBroadcast();
        ether.udpServerListenOnPort(udpCallback, port);
        _port = port;
        _isOpen = true;
        return true;
    }
    
    bool open(uint8_t* mac,
        const uint8_t* my_ip, const uint8_t* gw_ip = 0, const uint8_t* dns_ip = 0, const uint8_t* mask = 0, const uint8_t* broadcast_ip = 0,
        uint8_t csPin = 10, uint16_t port = 5005) {

        if (_isOpen) {
            Serial.println(F("Already open"));
            return true;
        }
        // ether is a global instantiation
        if (!ether.begin(ETHER_BUFFER_SIZE, mac, csPin)) {
            Serial.println(F("Failed to access ENC28J60"));
            return false;
        }
        // Static IP mode
        if (!ether.staticSetup(my_ip, gw_ip, dns_ip, mask)) {
            Serial.println(F("Failed to set static IP"));
            return false;
        }
        ether.enableBroadcast();
        ether.udpServerListenOnPort(udpCallback, port);
        if (broadcast_ip != 0) {
            for (size_t byte_i = 0; byte_i < 4; ++byte_i) {
                _broadcastIp[byte_i] = broadcast_ip[byte_i];
            }
        }
        _port = port;
        _isOpen = true;
        return true;
    }

    bool open(uint16_t port = 5005) override {
        uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
        return this->open(mac);
    }

    //--- REINITIALIZE WITH NEW SETTINGS ---//
    // Just call begin() again! No disable() needed.
    void close() override {
        _isOpen = false;
    }

    bool send(const char* data, size_t size, const uint8_t* source_ip = 0) override {

        if (source_ip == 0 || true) {
            ether.sendUdp(data, size, _port, _broadcastIp, _port);
        } else {
            ether.sendUdp(data, size, _port, source_ip, _port);
        }

        #ifdef BROADCAST_SOCKET_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);   // Properly prints raw bytes as characters
        Serial.println();           // Adds newline after the printed data
        #endif

        return true;
    }

    void receive() override {    // Just a trigger
        ether.packetLoop(ether.packetReceive());
    }

};  

uint8_t BroadcastSocket_EtherCard::_broadcastIp[] = {255,255,255,255};
uint16_t BroadcastSocket_EtherCard::_port = 5005;
BroadcastSocket_EtherCard broadcast_socket;

#endif // BROADCAST_SOCKET_ETHERCARD_HPP
