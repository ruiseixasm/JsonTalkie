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
                receive(doc["message"]);
            }
        }
    }
}

bool JsonTalkie::receive(JsonObjectConst message) {
    const char* type = message["type"];
    
    if (!type) return false;

    if (strcmp(type, "talk") == 0) {
        DynamicJsonDocument echoDoc(256);
        JsonObject echo = echoDoc.to<JsonObject>();
        echo["type"] = "echo";
        echo["to"] = message["from"];
        echo["id"] = message["id"];
        
        if (message.containsKey("to")) {
            // Handle commands (run/set/get)
            if (_manifesto.containsKey("run")) {
                for (JsonPair kv : _manifesto["run"].as<JsonObject>()) {
                    echo["response"] = "[run " + _manifesto["talker"]["name"].as<String>() + " " + kv.key().c_str() + "]\t" + kv.value()["description"].as<String>();
                    talk(echo);
                }
            }
            // Similar blocks for "set" and "get"...
        } else {
            echo["response"] = "[" + _manifesto["talker"]["name"].as<String>() + "]\t" + _manifesto["talker"]["description"].as<String>();
            talk(echo);
        }
    }
    else if (strcmp(type, "run") == 0) {
        // Implement run command handling...
    }
    // Other message types...
    
    return false;
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
