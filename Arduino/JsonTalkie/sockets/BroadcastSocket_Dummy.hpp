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
        StaticJsonDocument<256> _sentMessage;

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
            
            deserializeJson(_sentMessage, talk);
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
                    const char* messages[] = {
                        R"({"type":"talk","from":"Dummy"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"buzz"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"light_on"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"light_off"})"
                    };
                    const size_t num_messages = sizeof(messages)/sizeof(char*);
                    
                    // 3. Safer Random Selection
                    const char* message = messages[random(num_messages)];
        
                    // 4. Buffer Copy with Validation
                    size_t message_size = strlen(message);
                    size_t read_size = min(size - 1, message_size); // R"()" misses the '\0' demanding ad-oc insertion
                    
                    memcpy(buffer, message, read_size);
                    // R"()" format demands a ending '\0' ad-oc insertion
                    buffer[read_size] = '\0';
        
                    // 5. JSON Handling with Memory Checks
                    {
                        StaticJsonDocument<256> talk_doc;
                        if (talk_doc.capacity() == 0) {
                            Serial.println("Failed to allocate JSON talk_doc");
                            return 0;
                        }

                        JsonObject talk_json = talk_doc.to<JsonObject>();

                        // // Create message sub-object
                        // JsonObject message_json = talk_json.createNestedObject("message");
                        // deserializeJson(message_json, message);  // Parse into nested object
                        // // message_json["id"] = "4bc70d90";
    
                        // talk_json["checksum"] = message_checksum(buffer, read_size);
        
                        talk_json["message"] = message;
                        talk_json["checksum"] = message_checksum(buffer, read_size);
        
                        // 6. Safer Serialization
                        char json_buffer[256];
                        size_t json_len = serializeJson(talk_doc, json_buffer);
                        
                        if (json_len == 0 || json_len >= sizeof(json_buffer)) {
                            Serial.println("Serialization failed/buffer overflow");
                            return 0;
                        }
        
                        Serial.print("DUMMY READ: ");
                        Serial.println(json_buffer);
                        // serializeJsonPretty(talk_doc, Serial);  // Pretty-print for verification
                        // Serial.println();
                    } // JSON talk_doc freed here
        
                    return read_size;
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
        
        static uint16_t message_checksum(const char* message, size_t len) {
            // 16-bit word and XORing
            uint16_t checksum = 0;
            for (size_t i = 0; i < len; i += 2) {
                uint16_t chunk = message[i] << 8;
                if (i + 1 < len) {
                    chunk |= message[i + 1];
                }
                checksum ^= chunk;
            }
            return checksum;
        }
    };

#endif // BROADCAST_SOCKET_DUMMY_HPP
