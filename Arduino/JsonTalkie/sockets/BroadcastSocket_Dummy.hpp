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
#ifndef BROADCAST_SOCKET_DUMMY_HPP
#define BROADCAST_SOCKET_DUMMY_HPP

#include "../BroadcastSocket.hpp"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <ArduinoJson.h>


// To occupy less Flash memory
#define ARDUINO_JSON_VERSION 6

// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128
#define BROADCAST_SOCKET_DEBUG


class BroadcastSocket_Dummy : public BroadcastSocket {
private:
    unsigned long _lastTime = 0;
    char _buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

    // Helper function to safely create char* from buffer
    static char* decode(const uint8_t* data, const size_t length, char* talk) {
        memcpy(talk, data, length);
        talk[length] = '\0';
        return talk;
    }

    bool valid_checksum(JsonObject message) {
        // Use a static buffer size, large enough for your JSON
        uint16_t message_checksum = 0;
        if (message.containsKey("c")) {
            message_checksum = message["c"];
        }
        message["c"] = 0;
        size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        // 16-bit word and XORing
        uint16_t checksum = 0;
        for (size_t i = 0; i < len; i += 2) {
            uint16_t chunk = _buffer[i] << 8;
            if (i + 1 < len) {
                chunk |= _buffer[i + 1];
            }
            checksum ^= chunk;
        }
        // Serial.print("Message checksum: ");
        // Serial.println(checksum);  // optional: just to add a newline after the JSON
        message["c"] = checksum;
        return message_checksum == checksum;
    }
public:
    ~BroadcastSocket_Dummy() override = default;

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
        _isOpen = true;
        return _isOpen;
    }

    void close() override {
        _isOpen = false;
    }

    bool send(const char* data, size_t len, const uint8_t* source_ip = 0) override {
        if (!_isOpen)
            return false;
            
        #ifdef BROADCAST_SOCKET_DEBUG   
        Serial.print(F("DUMMY SENT: "));
        char talk[len + 1];
        Serial.println(decode(data, len, talk));
        #endif

        return true;
    }

    void receive() override {
        if (millis() - _lastTime > 1000) {
            _lastTime = millis();
            if (random(1000) < 100) { // 10% chance
                // 2. Message Selection
                // ALWAYS VALIDATE THE MESSAGES FOR BAD FORMATING !!
                const char* PROGMEM messages[] = {
                    R"({"m":0,"f":"Dummy","i":3003412860})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","w":"buzz","i":3003412861})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","w":"on","i":3003412862})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","w":"off","i":3003412863})"
                };
                const size_t num_messages = sizeof(messages)/sizeof(char*);
                
                // 3. Safer Random Selection
                const char* message_char = messages[random(num_messages)];
                size_t message_size = strlen(message_char);
                
                // 5. JSON Handling with Memory Checks
                {
                    #if ARDUINO_JSON_VERSION == 6
                    StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
                    if (message_doc.capacity() == 0) {
                        Serial.println(F("Failed to allocate JSON message_doc"));
                        return;
                    }
                    #else
                    JsonDocument message_doc;
                    if (message_doc.overflowed()) {
                        Serial.println(F("Failed to allocate JSON message_doc"));
                        return;
                    }
                    #endif
                    
                    // Message needs to be '\0' terminated and thus buffer is used instead
                    // it's possible to serialize from a JsonObject but it isn't to deserialize into a JsonObject!
                    DeserializationError error = deserializeJson(message_doc, message_char, message_size);
                    if (error) {
                        Serial.println(F("Failed to deserialize message"));
                        return;
                    }
                    JsonObject message = message_doc.as<JsonObject>();
                    valid_checksum(message);
    
                    size_t message_len = serializeJson(message, _buffer);
                    if (message_len == 0 || message_len >= BROADCAST_SOCKET_BUFFER_SIZE) {
                        Serial.println(F("Serialization failed/buffer overflow"));
                        return;
                    }

                    #ifdef BROADCAST_SOCKET_DEBUG    
                    Serial.print("DUMMY READ: ");
                    serializeJson(message, Serial);
                    Serial.println();  // optional: just to add a newline after the JSON
                    #endif

                    _socketCallback(_buffer, message_len);
                
                } // JSON talk_doc freed here
            }
        }
    }

};

BroadcastSocket_Dummy broadcast_socket;

#endif // BROADCAST_SOCKET_DUMMY_HPP
