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

// To occupy less Flash memory
#define ARDUINO_JSON_VERSION 6

// Readjust if absolutely necessary
#define JSON_TALKIE_BUFFER_SIZE 128

// Keys:
//     m: message
//     t: to
//     f: from
//     i: id
//     r: reply
//     w: what
//     s: checksum
//     v: value
//     l: list

namespace JsonTalkie {

    // MANIFESTO PROTOTYPING

    struct Device {
        const char* name;      // Name of the Device (Talker)
        const char* desc;      // Description of the Device
    };

    struct Run {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (*function)(JsonObject);  // Function pointer (no args)
    };

    struct Set {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        bool (*function)(JsonObject, int);  // Function pointer (const char*)
    };

    struct Get {
        const char* name;      // "buzz", "print", etc.
        const char* desc;      // Description
        int (*function)(JsonObject);  // Function pointer (no args)
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
        static bool (*echo)(JsonObject);

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
        char _buffer[JSON_TALKIE_BUFFER_SIZE] = {'\0'};
        char _sent_message_id[9] = {'\0'};  // 8 chars + null terminator
        unsigned long _messageTime;
        bool _running;

    private:
        static char* generateMessageId(char* buffer, size_t size) {
            // Combine random values with system metrics for better uniqueness
            uint32_t r1 = random(0xFFFF);
            uint32_t r2 = random(0xFFFF);
            uint32_t r3 = millis() & 0xFFFF;
            uint32_t combined = (r1 << 16) | r2 ^ r3;
            // Equivalent to the Python: return uuid.uuid4().hex[:8]
            if (size < 9) return buffer;
            buffer[8] = '\0';  // JSON_TALKIE_BUFFER_SIZE sized
            snprintf(buffer, 9, "%08lx", combined);
            return buffer;
        }

        static bool valid_checksum(JsonObject message, char* buffer, size_t size) {
            // Use a static buffer size, large enough for your JSON
            uint16_t message_checksum = 0;
            if (message.containsKey("s")) {
                message_checksum = message["s"];
            }
            message["s"] = 0;
            // char buffer[JSON_TALKIE_BUFFER_SIZE];
            size_t len = serializeJson(message, buffer, size);   // JSON_TALKIE_BUFFER_SIZE sized
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
            message["s"] = checksum;
            return message_checksum == checksum;
        }

        static bool validateTalk(JsonObject message, char* buffer, size_t size) {
            if (!message.containsKey("s"))
                return false;
            // NEEDS TO BE COMPLETED
            return valid_checksum(message, buffer, size);
        }
        
        bool receive(JsonObject message) {
            if (!message["m"])
                return false;

            message["t"] = message["f"];
        
            if (message["m"] == "talk") {
                message["m"] = "echo";
                message["r"] = Manifesto::talk()->desc;
                talk(message);
            } else if (message["m"] == "run") {
                message["m"] = "echo";
                const Run* run = Manifesto::run(message["w"]);
                if (run == nullptr) {
                    message["r"] = "UNKNOWN";
                } else {
                    message["r"] = "ROGER";
                }
                talk(message);
                if (run != nullptr) {
                    run->function(message);
                }
            } else if (message["m"] == "set") {
                if (message.containsKey("v") && message["v"].is<int>()) {
                    message["m"] = "echo";
                    const Set* set = Manifesto::set(message["w"]);
                    if (set == nullptr) {
                        message["r"] = "UNKNOWN";
                    } else {
                        message["r"] = "ROGER";
                    }
                    talk(message);
                    if (set != nullptr) {
                        set->function(message, message["v"].as<int>());
                    }
                }
            } else if (message["m"] == "get") {
                message["m"] = "echo";
                const Get* get = Manifesto::get(message["w"]);
                if (get == nullptr) {
                    message["r"] = "UNKNOWN";
                } else {
                    message["r"] = "ROGER";
                    message["v"] = get->function(message);
                }
                talk(message);
            } else if (message["m"] == "sys") {
                message["m"] = "echo";

                // AVR Boards (Uno, Nano, Mega) - Check RAM size
                #ifdef __AVR__
                uint16_t ramSize = RAMEND - RAMSTART + 1;
                if (ramSize == 2048)
                    message["r"] = "Arduino Uno/Nano (ATmega328P)";
                else if (ramSize == 8192)
                    message["r"] = "Arduino Mega (ATmega2560)";
                else
                    message["r"] = "Unknown AVR Board";
              
                // ESP8266
                #elif defined(ESP8266)
                message["r"] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
                
                // ESP32
                #elif defined(ESP32)
                message["r"] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
                
                // ARM (Due, Zero, etc.)
                #elif defined(__arm__)
                message["r"] = "ARM-based Board";

                // Unknown Board
                #else
                message["r"] = "Unknown Board";

                #endif

                talk(message);
            } else if (message["m"] == "echo") {
                message["m"] = "echo";
                message["w"] = "echo";
                if (Manifesto::echo != nullptr) {
                    Manifesto::echo(message);
                }
            }
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
            if (!_running)
                return false;

            size_t len = 0;

            // In order to release memory when done
            {
                // Directly nest the editable message under "m"
                if (message.isNull()) {
                    Serial.println("Error: Null message received");
                    return false;
                }

                // Set default 'id' field if missing
                if (!message.containsKey("i")) {
                    message["i"] = generateMessageId(_buffer, JSON_TALKIE_BUFFER_SIZE);
                }
                message["f"] = Manifesto::talk()->name;
                valid_checksum(message, _buffer, JSON_TALKIE_BUFFER_SIZE);

                len = serializeJson(message, _buffer, JSON_TALKIE_BUFFER_SIZE);
                if (len == 0) {
                    Serial.println("Error: Serialization failed");
                } else {
                    if (message["m"] != "echo") {
                        strncpy(_sent_message_id, message["i"], sizeof(_sent_message_id) - 1); // Explicit copy
                        _sent_message_id[sizeof(_sent_message_id) - 1] = '\0'; // Ensure null-termination
                    }

                    // Serial.print("A: ");
                    // serializeJson(message, Serial);
                    // Serial.println();  // optional: just to add a newline after the JSON

                    return _socket->write((uint8_t*)_buffer, len);
                }
            }
            return false;
        }

        void listen() {
            if (!_running)
                return;
        
            if (_socket->available()) { // Data in the socket buffer

                size_t bytesRead = _socket->read(_buffer, JSON_TALKIE_BUFFER_SIZE - 1);
                
                if (bytesRead > 0) {
                    _buffer[bytesRead] = '\0';

                    Serial.print("L: ");
                    Serial.write(_buffer, bytesRead);  // Properly prints raw bytes as characters
                    Serial.println();            // Adds newline after the printed data

                    // Lives until end of function
                    #if ARDUINO_JSON_VERSION == 6
                    StaticJsonDocument<JSON_TALKIE_BUFFER_SIZE> message_doc;
                    if (message_doc.capacity() < JSON_TALKIE_BUFFER_SIZE) {  // Absolute minimum
                        Serial.println("CRITICAL: Insufficient RAM");
                        return;
                    }
                    #else
                    JsonDocument message_doc;
                    if (message_doc.overflowed()) {
                        Serial.println("Failed to allocate JSON message_doc");
                        return;
                    }
                    #endif

                    DeserializationError error = deserializeJson(message_doc, _buffer);
                    if (error) {
                        Serial.println("Failed to deserialize buffer");
                        return;
                    }
                    JsonObject message = message_doc.as<JsonObject>();

                    if (validateTalk(message, _buffer, JSON_TALKIE_BUFFER_SIZE)) {

                        Serial.print("Remote: ");
                        serializeJson(message, Serial);
                        Serial.println();  // optional: just to add a newline after the JSON
    
                        receive(message);
                    }
                }

            }
        }
    };
}

#endif
