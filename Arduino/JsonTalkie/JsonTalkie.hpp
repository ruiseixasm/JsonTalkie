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

// Readjust if absolutely necessary
#define JSON_TALKIE_SIZE 128

// Keys:
//     m: message
//     c: command
//     t: to
//     f: from
//     i: id
//     r: reply
//     w: what
//     s: checksum

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
        bool (*function)(JsonObjectConst, JsonVariant);  // Function pointer (no args)
    };

    struct Set {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (*function)(JsonObjectConst, JsonVariant, size_t);  // Function pointer (const char*)
    };

    struct Get {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        size_t (*function)(JsonObjectConst, JsonVariant);  // Function pointer (no args)
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
        static bool (*echo)(JsonVariantConst);

        static const Device* talk() {
            return &Manifesto::device;
        }
    
        static const Run* run(const char* cmd) {
            for (int index = 0; index < Manifesto::runSize; ++index) {
                if (strcmp(cmd, Manifesto::runCommands[index].name) == 0) {
                    return &Manifesto::runCommands[index];  // Returns the function
                }
            }
            return nullptr;
        }
    
        static const Set* set(const char* cmd) {
            for (int index = 0; index < Manifesto::runSize; ++index) {
                if (strcmp(cmd, Manifesto::setCommands[index].name) == 0) {
                    return &Manifesto::setCommands[index];  // Returns the function
                }
            }
            return nullptr;
        }
    
        static const Get* get(const char* cmd) {
            for (int index = 0; index < Manifesto::runSize; ++index) {
                if (strcmp(cmd, Manifesto::getCommands[index].name) == 0) {
                    return &Manifesto::getCommands[index];  // Returns the function
                }
            }
            return nullptr;
        }
    };


    // JSONTALKIE DEFINITIONS

    class Talker {
    private:
        BroadcastSocket* _socket;
        char _sent_message_id[9]; // 8 chars + null terminator
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

        static const char* getId_8bytes() {
            // Combine random values with system metrics for better uniqueness
            uint32_t r1 = random(0xFFFF);
            uint32_t r2 = random(0xFFFF);
            uint32_t r3 = millis() & 0xFFFF;
            uint32_t combined = (r1 << 16) | r2 ^ r3;
            // Equivalent to the Python: return uuid.uuid4().hex[:8]
            static char buffer[9]; // 8 chars + null terminator
            buffer[8] = '\0';
            snprintf(buffer, sizeof(buffer), "%08lx", combined);
            return buffer;
        }


        static uint16_t calculateChecksum(JsonObjectConst message) {
            // Use a static buffer size, large enough for your JSON
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

            return checksum;
        }

        static bool validateTalk(JsonObjectConst talk) {
            if (!talk.containsKey("s") || !talk.containsKey("m")) {
                return false;
            }
            
            uint16_t checksum = talk["s"];
            JsonObjectConst message = talk["m"];
            return checksum == calculateChecksum(message);
        }
        
        bool receive(JsonObject message) {
            const char* command = message["c"];
            
            if (!command) return false;
        
            if (strcmp(command, "talk") == 0) {
                message["c"] = "echo";
                message["t"] = message["f"];
                message["r"] = Manifesto::talk()->desc;
                talk(message);

                // StaticJsonDocument<JSON_TALKIE_SIZE> echo_soc;
                // JsonObject echo = echo_soc.to<JsonObject>();    // echo_soc.to releases memory and resets echo_soc
                // echo["r"] = Manifesto::talk()->desc;
                // echo["c"] = "echo";
                // echo["t"] = message["f"];
                // echo["i"] = message["i"];
                // talk(echo);
            } else if (strcmp(command, "run") == 0) {
                message["c"] = "echo";
                message["t"] = message["f"];
                const Run* run = Manifesto::run(message["w"]);
                if (run == nullptr) {
                    message["r"] = "UNKNOWN";
                } else {
                    message["r"] = "ROGER";
                }
                talk(message);
                if (run != nullptr) {
                    run->function(message, message["r"]);
                }

                // StaticJsonDocument<JSON_TALKIE_SIZE> echo_soc;
                // JsonObject echo = echo_soc.to<JsonObject>();    // echo_soc.to releases memory and resets echo_soc
                // echo["c"] = "echo";
                // echo["t"] = message["f"];
                // echo["i"] = message["i"];
                // const Run* run = Manifesto::run(message["w"]);
                // if (run == nullptr) {
                //     echo["r"] = "UNKNOWN";
                // } else {
                //     echo["r"] = "ROGER";
                // }
                // talk(echo);
                // if (run != nullptr) {
                //     run->function(message, echo["r"]);
                // }
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

        bool talk(JsonObject message) {

            char buffer[JSON_TALKIE_SIZE];
            size_t len = 0;

            // In order to release memory when done
            {
                StaticJsonDocument<JSON_TALKIE_SIZE> talk_doc;
                JsonObject talk_json = talk_doc.to<JsonObject>();

                // Directly nest the editable message under "m"
                talk_json["m"] = message;  // No copy needed (refers to original)

                // Set default 'id' field if missing
                if (!talk_json["m"].containsKey("i")) {
                    talk_json["m"]["i"] = generateMessageId();
                }
                talk_json["m"]["f"] = Manifesto::talk()->name;
                
                talk_json["s"] = calculateChecksum(talk_json["m"]);
                len = serializeJson(talk_json, buffer, sizeof(buffer));

                strncpy(_sent_message_id, talk_json["m"]["i"], sizeof(_sent_message_id) - 1); // Explicit copy
                _sent_message_id[sizeof(_sent_message_id) - 1] = '\0'; // Ensure null-termination
            }
            
            return _socket->write((uint8_t*)buffer, len);
        }

        void listen() {
            if (!_running) return;
        
            if (_socket->available()) {
                uint8_t buffer[JSON_TALKIE_SIZE];
                size_t bytesRead = _socket->read(buffer, sizeof(buffer) - 1);
                
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    StaticJsonDocument<JSON_TALKIE_SIZE> talk_doc;
                    DeserializationError error = deserializeJson(talk_doc, (const char*)buffer);
                    
                    Serial.print("Z: ");
                    serializeJson(talk_doc["m"], Serial);
                    Serial.println();  // optional: just to add a newline after the JSON

                    if (!error && validateTalk(talk_doc.as<JsonObject>())) {
                        receive(talk_doc["m"]);
                    }
                }
            }
        }




    };
}

#endif
