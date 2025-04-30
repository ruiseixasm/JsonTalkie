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
#include "JsonTalkie.h"
#include "BroadcastSocket.h"

JsonTalkie::JsonTalkie(BroadcastSocket* socket, const char* manifesto) 
    : _socket(socket), _manifesto(1024), _lastMessage(256) {
    deserializeJson(_manifesto, manifesto);
    _running = false;
    _messageTime = 0;
}

bool JsonTalkie::begin() {
    if (!_socket->begin()) {
        return false;
    }
    _running = true;
    return true;
}

void JsonTalkie::end() {
    _running = false;
    _socket->end();
}

bool JsonTalkie::talk(JsonObjectConst message) {
    DynamicJsonDocument doc(256);
    JsonObject talk = doc.to<JsonObject>();
    
    // Create a copy of the message to modify
    JsonObject msgCopy = talk.createNestedObject("message");
    for (JsonPairConst kv : message) {
        msgCopy[kv.key()] = kv.value();
    }
    
    // Set default fields if missing
    if (!msgCopy.containsKey("from")) {
        msgCopy["from"] = _manifesto["talker"]["name"].as<String>();
    }
    if (!msgCopy.containsKey("id")) {
        msgCopy["id"] = generateMessageId();
    }
    
    talk["checksum"] = calculateChecksum(msgCopy);
    
    String output;
    serializeJson(talk, output);
    return _socket->write((const uint8_t*)output.c_str(), output.length());
}

void JsonTalkie::listen() {
    if (!_running) return;

    if (_socket->available()) {
        uint8_t buffer[256];
        size_t bytesRead = _socket->read(buffer, sizeof(buffer)-1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, (const char*)buffer);
            
            if (!error && validateTalk(doc.as<JsonObject>())) {
                processMessage(doc["message"]);
            }
        }
    }
}

bool JsonTalkie::receive(JsonObjectConst message) {
    // Implementation of message processing
    // Similar to Python version but adapted for ArduinoJson
    // ...
    return true;
}

bool JsonTalkie::validateTalk(JsonObjectConst talk) {
    if (!talk.containsKey("checksum") || !talk.containsKey("message")) {
        return false;
    }
    
    uint16_t checksum = talk["checksum"];
    JsonObjectConst message = talk["message"];
    return checksum == calculateChecksum(message);
}

String JsonTalkie::generateMessageId() {
    // Simple ID generation for Arduino
    return String(millis(), HEX) + "-" + String(random(0xFFFF), HEX);
}

uint16_t JsonTalkie::calculateChecksum(JsonObjectConst message) {
    String output;
    serializeJson(message, output);
    
    uint16_t checksum = 0;
    const char* str = output.c_str();
    for (size_t i = 0; str[i] != '\0'; i++) {
        checksum = (checksum << 5) + checksum + str[i];
    }
    return checksum;
}
