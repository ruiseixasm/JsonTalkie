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
        StaticJsonDocument<256> _lastMessage;
        uint8_t _receiveBuffer[256];
        size_t _receiveLength = 0;

        // Helper function to safely create char* from buffer
        static const char* decode(const uint8_t* data, size_t length) {
            char temp[length + 1];
            memcpy(temp, data, length);
            temp[length] = '\0';
            return temp;
        }

    public:
        ~BroadcastSocket_Dummy() override = default;
    
        bool begin() override {
            _isOpen = random(1000) > 50; // 95% success rate
            return _isOpen;
        }
    
        void end() override {
            _isOpen = false;
            _receiveLength = 0;
        }
    
        bool write(const uint8_t* data, size_t length) override {
            if (!_isOpen)
                return false;
                
            char message[length + 1];
            memcpy(message, data, length);
            message[length] = '\0';
            Serial.print("DUMMY SENT: ");
            Serial.println(message);
            
            deserializeJson(_lastMessage, message);
            return true;
        }
    
        bool available() override {
            if (!_isOpen) return false;
            
            if (millis() - _lastTime > 1000) {
                _lastTime = millis();
                if (random(1000) < 10) { // 1% chance to receive
                    const char* messages[] = {
                        R"({"type":"run","what":"buzz","to":"Buzzer","from":"Buzzer"})",
                        R"({"type":"echo","to":"Buzzer","response":"[Buzzer buzz]\\tCalled","from":"Buzzer"})",
                        R"({"type":"talk","from":"Dummy"})",
                        R"({"type":"echo","to":"Talker-a6","response":"[Talker-a6]\\tA simple Talker!","from":"Talker-a6"})"
                    };
                    
                    const char* chosen = messages[random(4)];
                    _receiveLength = strlen(chosen);
                    memcpy(_receiveBuffer, chosen, _receiveLength);
                    return true;
                }
            }
            return false;
        }
    
        size_t read(uint8_t* buffer, size_t size) override {
            if (!available() || size < _receiveLength) return 0;
            
            size_t toCopy = min(size, _receiveLength);
            memcpy(buffer, _receiveBuffer, toCopy);
            _receiveLength = 0; // Clear buffer after reading
            
            Serial.print("DUMMY RECEIVED: ");
            Serial.println((const char*)buffer);
            
            return toCopy;
        }
    
        static String generateMessageId() {
            char buf[9];
            snprintf(buf, sizeof(buf), "%08lx", random(0x10000000, 0xFFFFFFFF));
            return String(buf);
        }
    };

#endif // BROADCAST_SOCKET_DUMMY_HPP
