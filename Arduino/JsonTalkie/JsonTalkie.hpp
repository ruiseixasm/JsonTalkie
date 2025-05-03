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
#ifndef JSON_TALKIE_HPP
#define JSON_TALKIE_HPP

#include "BroadcastSocket.hpp"
#include <Arduino.h>
#include <ArduinoJson.h>    // Install ArduinoJson Library

namespace JsonTalkie {

    // HELPER METHODS

    // Place this ABOVE your Talker class definition
    char* floatToStr(float val, uint8_t decimals = 2) {
        static char buffer[16]; // Holds "-327.00" (large enough for most cases)
        dtostrf(val, 0, decimals, buffer); // Works on ALL Arduino boards
        return buffer;
    }


    // MANIFESTO PROTOTYPING

    struct Device {
        const char* name;      // Name of the Device (Talker)
        const char* desc;      // Description of the Device
    };

    struct Run {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        const char* (*function)();  // Function pointer (no args)
    };

    struct Set {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        const char* (*function)(const char*);  // Function pointer (const char*)
    };

    struct Get {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        const char* (*function)();  // Function pointer (no args)
    };

    // Structure Definition
    struct Manifesto {

        static const Device device;         // Declaration only
        static const Run runCommands[];     // Declaration only
        static const size_t runSize;        // Declaration only
        static const Set setCommands[];
        static const size_t setSize;        // Declaration only
        static const Get getCommands[];
        static const size_t getSize;        // Declaration only
        static bool (*echo)(StaticJsonDocument<256>*, const char*);

        static const Device* talk() {
            return &Manifesto::device;
        }
    
        static const char* run(const char* cmd) {
            for (int index = 0; index < Manifesto::runSize; ++index) {
                if (strcmp(cmd, Manifesto::runCommands[index].name) == 0) {
                    return (Manifesto::runCommands[index].function)();  // Call the function
                }
            }
            return "Command not found";
        }
    
        static const char* set(const char* cmd, const char* value) {
            for (int index = 0; index < Manifesto::runSize; ++index) {
                if (strcmp(cmd, Manifesto::setCommands[index].name) == 0) {
                    return (Manifesto::setCommands[index].function)(value);  // Call the function
                }
            }
            return "Command not found";
        }
    
        static const char* get(const char* cmd) {
            for (int index = 0; index < Manifesto::runSize; ++index) {
                if (strcmp(cmd, Manifesto::getCommands[index].name) == 0) {
                    return (Manifesto::getCommands[index].function)();  // Call the function
                }
            }
            return "Command not found";
        }
    };


    // JSONTALKIE DEFINITIONS

    class Talker {
    private:
        BroadcastSocket* _socket;
        StaticJsonDocument<256> _sentMessage;
        unsigned long _messageTime;
        bool _running;

    private:
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

        static bool validateTalk(JsonObjectConst talk) {
            if (!talk.containsKey("checksum") || !talk.containsKey("message")) {
                return false;
            }
            
            uint16_t checksum = talk["checksum"];
            JsonObjectConst message = talk["message"];
            return checksum == calculateChecksum(message);
        }
        
        bool receive(JsonObjectConst message) {
            const char* type = message["type"];
            
            if (!type) return false;
        
            if (strcmp(type, "talk") == 0) {
                StaticJsonDocument<256> echo_soc;
                char response[256]; // Adjust size as needed
                snprintf(response, sizeof(response), "[%s]\t%s", Manifesto::talk()->name, Manifesto::talk()->desc);
                JsonObject echo = echo_soc.to<JsonObject>();    // echo_soc.to releases memory and resets echo_soc
                echo["response"] = response;
                echo["type"] = "echo";
                echo["to"] = message["from"];
                echo["id"] = message["id"];
                talk(echo);
            } else if (strcmp(type, "run") == 0) {
                StaticJsonDocument<256> echo_soc;
                char response[256]; // Adjust size as needed
                snprintf(response, sizeof(response), "[%s]\tRUN", Manifesto::talk()->name);
                JsonObject echo = echo_soc.to<JsonObject>();    // echo_soc.to releases memory and resets echo_soc
                echo["response"] = response;
                echo["type"] = "echo";
                echo["to"] = message["from"];
                echo["id"] = message["id"];
                talk(echo);
                echo["response"] = Manifesto::run(message["what"]);
                talk(echo);
            }
            // Other message types...
            
            return false;
        }

    public:
        Talker(BroadcastSocket* socket) : _socket(socket) {}
        
        bool begin() {
            if (!_socket->begin()) {
                return false;
            }
            _running = true;
            return true;
        }

        void end() {
            _running = false;
            _socket->end();
        }

        bool talk(JsonObjectConst message) {

            // Serial.print("A: ");
            // serializeJson(message, Serial);
            // Serial.println();  // optional: just to add a newline after the JSON

            StaticJsonDocument<256> doc;
            JsonObject talk_json = doc.to<JsonObject>();
            // Create a copy of the message to modify
            JsonObject message_json = talk_json.createNestedObject("message");

            for (JsonPairConst kv : message) {
                message_json[kv.key()] = kv.value();
            }
            // Set default fields if missing
            if (!message_json.containsKey("id")) {
                message_json["id"] = generateMessageId();
            }
            message_json["from"] = Manifesto::talk()->name;
            
            talk_json["checksum"] = calculateChecksum(message_json);

            String output;
            serializeJson(talk_json, output);
            return _socket->write((const uint8_t*)output.c_str(), output.length());
        }

        void listen() {
            if (!_running) return;
        
            if (_socket->available()) {
                uint8_t buffer[256];
                size_t bytesRead = _socket->read(buffer, sizeof(buffer)-1);
                
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    StaticJsonDocument<256> doc;
                    DeserializationError error = deserializeJson(doc, (const char*)buffer);
                    
                    if (!error && validateTalk(doc.as<JsonObject>())) {
                        receive(doc["message"]);
                    }
                }
            }
        }




    };
}

#endif
