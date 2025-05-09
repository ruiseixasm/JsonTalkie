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


// Readjust if absolutely necessary
#define JSON_TALKIE_BUFFER_SIZE 128
#define BROADCAST_SOCKET_DEBUG

class BroadcastSocket_Serial : public BroadcastSocket {
private:
    char _buffer[JSON_TALKIE_BUFFER_SIZE] = {'\0'};

public:
    
    // Arduino default SPI pins in https://docs.arduino.cc/language-reference/en/functions/communication/SPI/
    bool open(const uint8_t* mac, uint8_t csPin = 10, uint16_t port = 5005) {
        return open(port);
    }

    bool open(const uint8_t* mac,
        const uint8_t* my_ip, const uint8_t* gw_ip = 0, const uint8_t* dns_ip = 0, const uint8_t* mask = 0, const uint8_t* broadcast_ip = 0,
        uint8_t csPin = 10, uint16_t port = 5005) {

        return open(port);
    }

    bool open(uint16_t port = 5005) override {
        return true;
    }
    void close() override {
        Serial.end();
    }
    bool send(const char* data, size_t len, const uint8_t* source_ip = 0) override {
        return Serial.write(data, len) == len;
    }

    void receive() override {
        size_t message_len = Serial.readBytes(_buffer, JSON_TALKIE_BUFFER_SIZE);
        if (message_len > 0) {
            _socketCallback(_buffer, message_len);
        }
    }
};

BroadcastSocket_Serial broadcast_socket;

#endif // BROADCAST_SOCKET_SERIAL_HPP
