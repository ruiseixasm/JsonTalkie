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


class BroadcastSocket_Dummy {
private:
    bool _socket = false;
    unsigned long _lastTime = 0;
    StaticJsonDocument<256> _sentMessage;

    const char* _messages[4] = {
        "{\"type\":\"run\",\"what\":\"buzz\",\"to\":\"Buzzer\",\"from\":\"Buzzer\"}",
        "{\"type\":\"echo\",\"to\":\"Buzzer\",\"response\":\"[Buzzer buzz]\\tCalled\",\"from\":\"Buzzer\"}",
        "{\"type\":\"talk\",\"from\":\"Dummy\"}",
        "{\"type\":\"echo\",\"to\":\"Talker-a6\",\"response\":\"[Talker-a6]\\tA simple Talker!\",\"from\":\"Talker-a6\"}"
    };

public:
    bool open() {
        _socket = random(1000) > 50;
        return _socket;
    }

    void close() { _socket = false; }

    bool send(const String& data) {
        if (!_socket) return false;
        deserializeJson(_sentMessage, data);
        Serial.print("DUMMY SENT: ");
        Serial.println(data);
        return true;
    }

    String receive() {
        if (!_socket || millis() - _lastTime < 1000) return "";
        _lastTime = millis();
        
        if (random(1000) >= 10) return "";
        
        String message = _messages[random(4)];
        message.replace("\"id\":\"", "\"id\":\"" + message_id());
        
        StaticJsonDocument<256> doc;
        deserializeJson(doc, message);
        uint16_t chk = checksum(doc.as<JsonObject>());
        
        doc["checksum"] = chk;
        String output;
        serializeJson(doc, output);
        
        Serial.print("DUMMY RECEIVED: ");
        Serial.println(output);
        return output;
    }

    static String message_id() {
        char buf[9];
        sprintf(buf, "%08lx", random());
        return String(buf);
    }

    static uint16_t checksum(const JsonObject& obj) {
        String s;
        serializeJson(obj, s);
        uint16_t sum = 0;
        for (size_t i = 0; i < s.length(); i++) {
            sum = (sum << 5) + sum + s[i];
        }
        return sum;
    }
};
    

#endif // BROADCAST_SOCKET_DUMMY_HPP
