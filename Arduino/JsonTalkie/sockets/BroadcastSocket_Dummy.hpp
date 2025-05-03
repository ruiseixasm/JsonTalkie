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
            if (!_isOpen)
                return 0;

            if (millis() - _lastTime > 1000) {
                _lastTime = millis();
                if (random(1000) < 10) { // 1% chance to receive
                    const char* messages[] = {
                        R"({"type":"talk","from":"Dummy"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"buzz"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"light_on"})",
                        R"({"type":"run","from":"Dummy","to":"Buzzer","what":"light_off"})"
                    };
                    
                    const char* message = messages[random(4)];
                    size_t message_size = strlen(message);
                    size_t receive_size = min(size, message_size);
                    memcpy(buffer, message, receive_size);

                    Serial.print("DUMMY READ: ");
                    Serial.println((const char*)buffer);
                    
                    return receive_size;
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
    };

#endif // BROADCAST_SOCKET_DUMMY_HPP
