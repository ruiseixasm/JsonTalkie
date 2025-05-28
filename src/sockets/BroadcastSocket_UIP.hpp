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
#ifndef BROADCAST_SOCKET_UIP_HPP
#define BROADCAST_SOCKET_UIP_HPP

#include "../BroadcastSocket.hpp"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <UIPEthernet.h>
#include <UIPUdp.h>  // If using UDP


// #define BROADCAST_UIP_DEBUG
#define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_UIP : public BroadcastSocket {
private:
    IPAddress _source_ip;   // By default it's used the broadcast IP
    EthernetUDP* _udp = nullptr;

    // Private constructor for singleton
    BroadcastSocket_UIP() {
        _source_ip = IPAddress(255, 255, 255, 255);   // By default it's used the broadcast IP
    }

public:
    // Singleton accessor
    static BroadcastSocket_UIP& instance() {
        static BroadcastSocket_UIP instance;
        return instance;
    }

    
    bool send(const char* data, size_t size, bool as_reply = false) override {
        if (_udp == nullptr) return false;

        IPAddress broadcastIP(255, 255, 255, 255);
        
        #ifdef ENABLE_DIRECT_ADDRESSING
        if (!_udp->beginPacket(as_reply ? _source_ip : broadcastIP, _port)) {
            #ifdef BROADCAST_UIP_DEBUG
            Serial.println(F("Failed to begin packet"));
            #endif
            return false;
        }
        #else
        if (!_udp->beginPacket(broadcastIP, _port)) {
            #ifdef BROADCAST_UIP_DEBUG
            Serial.println(F("Failed to begin packet"));
            #endif
            return false;
        }
        #endif

        size_t bytesSent = _udp->write(reinterpret_cast<const uint8_t*>(data), size);

        if (!_udp->endPacket()) {
            #ifdef BROADCAST_UIP_DEBUG
            Serial.println(F("Failed to end packet"));
            #endif
            return false;
        }

        #ifdef BROADCAST_UIP_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);
        Serial.println();
        #endif

        return true;
    }


    size_t receive(char* buffer, size_t size) override {
        if (_udp == nullptr) return 0;
        // Receive packets
        int packetSize = _udp->parsePacket();
        if (packetSize > 0) {
            int length = _udp->read(buffer, min(static_cast<size_t>(packetSize), size));
            if (length <= 0) return 0;  // Your requested check - handles all error cases
            
            #ifdef BROADCAST_UIP_DEBUG
            Serial.print(packetSize);
            Serial.print(F("B from "));
            Serial.print(_udp->remoteIP());
            Serial.print(F(":"));
            Serial.print(_udp->remotePort());
            Serial.print(F(" -> "));
            Serial.println(buffer);
            #endif
            
            _source_ip = _udp->remoteIP();
            return jsonStrip(buffer, static_cast<size_t>(length));
        }
        return 0;   // nothing received
    }

    void set_udp(EthernetUDP* udp) { _udp = udp; }
};


#endif // BROADCAST_SOCKET_UIP_HPP
