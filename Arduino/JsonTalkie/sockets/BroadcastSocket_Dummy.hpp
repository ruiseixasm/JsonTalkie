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
#define JSON_TALKIE_SIZE 128


class BroadcastSocket_Dummy : public BroadcastSocket {
    private:
        bool _isOpen = false;
        unsigned long _lastTime = 0;

        // Helper function to safely create char* from buffer
        static char* decode(const uint8_t* data, const size_t length, char* talk) {
            memcpy(talk, data, length);
            talk[length] = '\0';
            return talk;
        }

    public:
        ~BroadcastSocket_Dummy() override = default;
    
        bool begin() override {
            _isOpen = true;
            return _isOpen;
        }
    
        void end() override {
            _isOpen = false;
        }
    
        bool write(const uint8_t* data, size_t length) override {
            if (!_isOpen)
                return false;
                
            Serial.print("DUMMY SENT: ");
            char talk[length + 1];
            Serial.println(decode(data, length, talk));
            return true;
        }
    
        bool available() override {
            return _isOpen;
        }
    
        size_t read(uint8_t* buffer, size_t size) override {
            // 1. Initial Safeguards
            if (!_isOpen || !buffer || size == 0)
                return 0;
        
            if (millis() - _lastTime > 1000) {
                _lastTime = millis();
                if (random(1000) < 100) { // 10% chance
                    // 2. Message Selection
                    // ALWAYS VALIDATE THE MESSAGES FOR BAD FORMATING !!
                    const char* messages[] = {
                        R"({"c":"talk","f":"Dummy","i":"4bc70d90"})",
                        R"({"c":"run","f":"Dummy","t":"Buzzer","w":"buzz","i":"4bc70d91"})",
                        R"({"c":"run","f":"Dummy","t":"Buzzer","w":"on","i":"4bc70d92"})",
                        R"({"c":"run","f":"Dummy","t":"Buzzer","w":"off","i":"4bc70d93"})"
                    };
                    const size_t num_messages = sizeof(messages)/sizeof(char*);
                    
                    // 3. Safer Random Selection
                    const char* message_char = messages[random(num_messages)];
                    size_t message_size = strlen(message_char);
                    
                    // 5. JSON Handling with Memory Checks
                    {
                        #if ARDUINO_JSON_VERSION == 6
                        StaticJsonDocument<JSON_TALKIE_SIZE> message_doc;
                        if (message_doc.capacity() == 0) {
                            Serial.println("Failed to allocate JSON message_doc");
                            return 0;
                        }
                        #else
                        JsonDocument message_doc;
                        if (message_doc.overflowed()) {
                            Serial.println("Failed to allocate JSON message_doc");
                            return 0;
                        }
                        #endif
                        
                        // Message needs to be '\0' terminated and thus buffer is used instead
                        // it's possible to serialize from a JsonObject but it isn't to deserialize into a JsonObject!
                        DeserializationError error = deserializeJson(message_doc, message_char, message_size);
                        if (error) {
                            Serial.println("Failed to deserialize message");
                            return 0;
                        }
                        JsonObject message = message_doc.as<JsonObject>();
                        checksum(message);
        
                        size_t message_len = serializeJson(message, buffer, size);
                        if (message_len == 0 || message_len >= size) {
                            Serial.println("Serialization failed/buffer overflow");
                            return 0;
                        }

                        // Serial.print("DUMMY READ: ");
                        // serializeJson(message, Serial);
                        // Serial.println();  // optional: just to add a newline after the JSON

                        return message_len;
                    
                    } // JSON talk_doc freed here
                }
            }
            return 0;
        }
    
        static String generateMessageId() {
            // Combine random values with system metrics for better uniqueness
            uint32_t r1 = random(0xFFFF);
            uint32_t r2 = random(0xFFFF);
            uint32_t r3 = millis() & 0xFFFF;
            uint32_t combined = (r1 << 16) | r2 ^ r3;
            // Equivalent to the Python: return uuid.uuid4().hex[:8]
            char buffer[9]; // 8 chars + null terminator
            snprintf(buffer, sizeof(buffer), "%08lx", combined);
            return String(buffer);
        }

        static bool checksum(JsonObject message) {
            // Use a static buffer size, large enough for your JSON
            bool equal_checksum = false;
            uint16_t message_checksum = 0;
            if (!message.containsKey("m")) {
                equal_checksum = true;
            } else {
                message_checksum = message["s"];
            }
            message["s"] = 0;
            char buffer[JSON_TALKIE_SIZE];
            size_t len = serializeJson(message, buffer);
            // 16-bit word and XORing
            uint16_t checksum = 0;
            for (size_t i = 0; i < len; i += 2) {
                uint16_t chunk = buffer[i] << 8;
                if (i + 1 < len) {
                    chunk |= buffer[i + 1];
                }
                checksum ^= chunk;
            }
            // Serial.print("Message checksum: ");
            // Serial.println(checksum);  // optional: just to add a newline after the JSON
            if (equal_checksum) {
                message["s"] = checksum;
                return true;
            }
            message["s"] = message_checksum;
            return message_checksum == checksum;
        }
    };

#endif // BROADCAST_SOCKET_DUMMY_HPP
