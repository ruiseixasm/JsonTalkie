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
#ifndef JSON_TALKIE_H
#define JSON_TALKIE_H

#include <Arduino.h>
#include <ArduinoJson.h>    // Install ArduinoJson Library

class BroadcastSocket; // Forward declaration

class JsonTalkie {
private:
    BroadcastSocket* _socket;
    DynamicJsonDocument _manifesto;
    DynamicJsonDocument _lastMessage;
    unsigned long _messageTime;
    bool _running;

    bool validateTalk(JsonObjectConst talk);

public:
    JsonTalkie(BroadcastSocket* socket, const char* manifesto);
    
    bool begin();
    void end();
    bool talk(JsonObjectConst message);
    void listen();
    bool receive(JsonObjectConst message);
    bool wait(unsigned long seconds = 2);

    static String generateMessageId();
    static uint16_t calculateChecksum(JsonObjectConst message);
    static String serialize(JsonObjectConst obj);
    static DynamicJsonDocument parse(const char* json);
};

#endif
