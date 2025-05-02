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
#ifndef BROADCAST_SOCKET_SERIAL_HPP
#define BROADCAST_SOCKET_SERIAL_HPP

#include "../BroadcastSocket.hpp"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!

class BroadcastSocket_Serial : public BroadcastSocket {
private:
    long _baudRate;

public:
    BroadcastSocket_Serial(long baud = 9600) : _baudRate(baud) {}
    
    bool begin() override {
        Serial.begin(_baudRate);
        return true;
    }
    void end() override {
        Serial.end();
    }
    bool write(const uint8_t* data, size_t length) override {
        return Serial.write(data, length) == length;
    }

    bool available() override {
        return Serial.available();
    }

    size_t read(uint8_t* buffer, size_t size) override {
        return Serial.readBytes((char*)buffer, size);
    }

};

#endif // BROADCAST_SOCKET_SERIAL_HPP
