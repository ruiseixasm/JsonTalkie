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
            _isOpen = random(1000) > 50; // 95% success rate
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
            if (!_isOpen || !buffer || size == 0) {
                return 0;
            }
        
            if (millis() - _lastTime > 1000) {
                _lastTime = millis();
                if (random(1000) < 100) { // 10% chance
                    // 2. Message Selection
                    // ALWAYS VALIDATE THE MESSAGES FOR BAD FORMATING !!
                    const char* messages[] = {
                        R"({"type":"talk","from":"Dummy","id":"4bc70d90"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"buzz","id":"4bc70d91"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"light_on","id":"4bc70d92"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"light_off","id":"4bc70d93"})"
                    };
                    const size_t num_messages = sizeof(messages)/sizeof(char*);
                    
                    // 3. Safer Random Selection
                    const char* message = messages[random(num_messages)];
                    size_t message_size = strlen(message);
                    
                    // 5. JSON Handling with Memory Checks
                    {
                        StaticJsonDocument<256> message_doc;
                        if (message_doc.capacity() == 0) {
                            Serial.println("Failed to allocate JSON message_doc");
                            return 0;
                        }
                        
                        // Message needs to be '\0' terminated and thus buffer is used instead
                        DeserializationError error = deserializeJson(message_doc, message, message_size);
                        if (error) {
                            Serial.println("Failed to deserialize message");
                            return 0;
                        }
                        
                        StaticJsonDocument<256> talk_doc;
                        if (talk_doc.capacity() == 0) {
                            Serial.println("Failed to allocate JSON talk_doc");
                            return 0;
                        }
                        JsonObject talk_json = talk_doc.to<JsonObject>();
                        talk_json.createNestedObject("message");

                        JsonObject message_json = message_doc.as<JsonObject>();
                        talk_json["message"] = message_json;
                        talk_json["checksum"] = calculateChecksum(message_json);
        
                        size_t json_len = serializeJson(talk_doc, buffer, size);

                        if (json_len == 0 || json_len >= size) {
                            Serial.println("Serialization failed/buffer overflow");
                            return 0;
                        }

                        char dummy_read[256];
                        serializeJson(talk_doc, dummy_read);
                        Serial.print("DUMMY READ: ");
                        Serial.println(dummy_read);

                        return json_len;
                    
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
        
        static uint16_t calculateChecksum(JsonObjectConst message) {
            // Use a static buffer size, large enough for your JSON
            char buffer[256];
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
            return checksum;
        }
    };

#endif // BROADCAST_SOCKET_DUMMY_HPP
