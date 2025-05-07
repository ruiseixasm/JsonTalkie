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
#define JSONTALKIE_DEBUG

// Keys:
//     c: checksum
//     d: description
//     f: from
//     i: id
//     m: message
//     n: name
//     r: reply
//     t: to
//     v: value
//     w: what

// Messages/Whats:
//     0 talk
//     1 list
//     2 run
//     3 set
//     4 get
//     5 sys
//     6 echo


namespace JsonTalkie {

    // MANIFESTO PROTOTYPING

    // Using PROGMEM to save strings in Flash memory instead of the RAM

    struct Device {
        const char* PROGMEM name;      // Name of the Device (Talker)
        const char* PROGMEM desc;      // Description of the Device
    };

    struct Run {
        const char* PROGMEM name;      // "buzz", "print", etc.
        const char* PROGMEM desc;      // Description
        bool (*function)(JsonObject);  // Function pointer (no args)
    };

    struct Set {
        const char* PROGMEM name;      // "buzz", "print", etc.
        const char* PROGMEM desc;      // Description
        bool (*function)(JsonObject, int);  // Function pointer (const char*)
    };

    struct Get {
        const char* PROGMEM name;      // "buzz", "print", etc.
        const char* PROGMEM desc;      // Description
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
        // Compiler reports these static RAM allocation
        #if ARDUINO_JSON_VERSION == 6
        static StaticJsonDocument<JSON_TALKIE_BUFFER_SIZE> _message_doc;
        #else
        static JsonDocument _message_doc;
        #endif
        static char _buffer[JSON_TALKIE_BUFFER_SIZE];
        static char _sent_message_id[9];  // 8 chars + null terminator
        static bool _running;

    private:
        static char* generateMessageId() {
            // Combine random values with system metrics for better uniqueness
            uint32_t r1 = random(0xFFFF);
            uint32_t r2 = random(0xFFFF);
            uint32_t r3 = millis() & 0xFFFF;
            uint32_t combined = (r1 << 16) | r2 ^ r3;
            // Equivalent to the Python: return uuid.uuid4().hex[:8]
            if (JSON_TALKIE_BUFFER_SIZE < 9) return _buffer;
            _buffer[8] = '\0';  // JSON_TALKIE_BUFFER_SIZE sized
            snprintf(_buffer, 9, "%08lx", combined);
            return _buffer;
        }

        static bool valid_checksum(JsonObject message) {
            // Use a static buffer size, large enough for your JSON
            uint16_t message_checksum = 0;
            if (message.containsKey("c")) {
                message_checksum = message["c"];
            }
            message["c"] = 0;
            size_t len = serializeJson(message, _buffer, JSON_TALKIE_BUFFER_SIZE);
            // 16-bit word and XORing
            uint16_t checksum = 0;
            for (size_t i = 0; i < len; i += 2) {
                uint16_t chunk = _buffer[i] << 8;
                if (i + 1 < len) {
                    chunk |= _buffer[i + 1];
                }
                checksum ^= chunk;
            }
            // Serial.print("Message checksum: ");
            // Serial.println(checksum);  // optional: just to add a newline after the JSON
            message["c"] = checksum;
            return message_checksum == checksum;
        }

        static bool validateTalk(JsonObject message) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println("Validating...");
            #endif
            if (!(message.containsKey("c") && message.containsKey("m")
                    && message.containsKey("i") && message.containsKey("f"))) {
                #ifdef JSONTALKIE_DEBUG
                Serial.println("NOT validated");
                #endif
                return false;
            }
            // NEEDS TO BE COMPLETED
            #ifdef JSONTALKIE_DEBUG
            Serial.println("Validated");
            #endif
            return valid_checksum(message);
        }
        
        static void listenCallback(const char* data, size_t length) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println("Callback...");
            #endif
            if (!_running)
                return;
        
            #ifdef JSONTALKIE_DEBUG
            Serial.println("Running...");
            #endif

            if (length < JSON_TALKIE_BUFFER_SIZE - 1) {
                memcpy(_buffer, data, length);
                _buffer[length] = '\0';

                #ifdef JSONTALKIE_DEBUG
                Serial.print("L: ");
                Serial.write(_buffer, length);  // Properly prints raw bytes as characters
                Serial.println();            // Adds newline after the printed data
                #endif

                DeserializationError error = deserializeJson(_message_doc, _buffer);
                if (error) {
                    #ifdef JSONTALKIE_DEBUG
                    Serial.println("Failed to deserialize buffer");
                    #endif
                    return;
                }
                JsonObject message = _message_doc.as<JsonObject>();

                if (validateTalk(message)) {

                    #ifdef JSONTALKIE_DEBUG
                    Serial.print("Received: ");
                    serializeJson(message, Serial);
                    Serial.println();  // optional: just to add a newline after the JSON
                    #endif

                    receive(message);
                }
            }
        }

        static bool receive(JsonObject message) {
            message["t"] = message["f"];
        
            if (message["m"] == 0) {            // talk
                message["m"] = 6;
                message["d"] = Manifesto::talk()->desc;
                return talk(message);
            } else if (message["m"] == 1) {     // list
                message["m"] = 6;
                message["w"] = 2;
                for (size_t run_i = 0; run_i < Manifesto::runSize; ++run_i) {
                    message["n"] = Manifesto::runCommands[run_i].name;
                    message["d"] = Manifesto::runCommands[run_i].desc;
                    talk(message);
                }
                message["w"] = 3;
                for (size_t set_i = 0; set_i < Manifesto::setSize; ++set_i) {
                    message["n"] = Manifesto::setCommands[set_i].name;
                    message["d"] = Manifesto::setCommands[set_i].desc;
                    talk(message);
                }
                message["w"] = 4;
                for (size_t get_i = 0; get_i < Manifesto::getSize; ++get_i) {
                    message["n"] = Manifesto::getCommands[get_i].name;
                    message["d"] = Manifesto::getCommands[get_i].desc;
                    talk(message);
                }
                return true;
            } else if (message["m"] == 2) {     // run
                message["m"] = 6;
                if (message.containsKey("n")) {
                    const Run* run = Manifesto::run(message["n"]);
                    if (run == nullptr) {
                        message["r"] = "UNKNOWN";
                    } else {
                        message["r"] = "ROGER";
                    }
                    talk(message);
                    if (run != nullptr) {
                        run->function(message);
                    }
                    return true;
                }
            } else if (message["m"] == 3) {     // set
                if (message.containsKey("n") && message.containsKey("v") && message["v"].is<int>()) {
                    message["m"] = 6;
                    const Set* set = Manifesto::set(message["n"]);
                    if (set == nullptr) {
                        message["r"] = "UNKNOWN";
                    } else {
                        message["r"] = "ROGER";
                    }
                    talk(message);
                    if (set != nullptr) {
                        set->function(message, message["v"].as<int>());
                    }
                    return true;
                }
            } else if (message["m"] == 4) {     // get
                message["m"] = 6;
                if (message.containsKey("n")) {
                    const Get* get = Manifesto::get(message["n"]);
                    if (get == nullptr) {
                        message["r"] = "UNKNOWN";
                    } else {
                        message["r"] = "ROGER";
                        message["v"] = get->function(message);
                    }
                    return talk(message);
                }
            } else if (message["m"] == 5) {     // sys
                message["m"] = 6;

                // AVR Boards (Uno, Nano, Mega) - Check RAM size
                #ifdef __AVR__
                uint16_t ramSize = RAMEND - RAMSTART + 1;
                if (ramSize == 2048)
                    message["d"] = "Arduino Uno/Nano (ATmega328P)";
                else if (ramSize == 8192)
                    message["d"] = "Arduino Mega (ATmega2560)";
                else
                    message["d"] = "Unknown AVR Board";
              
                // ESP8266
                #elif defined(ESP8266)
                message["d"] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
                
                // ESP32
                #elif defined(ESP32)
                message["d"] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
                
                // ARM (Due, Zero, etc.)
                #elif defined(__arm__)
                message["d"] = "ARM-based Board";

                // Unknown Board
                #else
                message["d"] = "Unknown Board";

                #endif

                return talk(message);
            } else if (message["m"] == 6) {     // echo
                if (Manifesto::echo != nullptr) {
                    Manifesto::echo(message);
                    return true;
                }
            }
            return false;
        }

    public:
        static bool begin() {
            if (!broadcast_socket.open()) {
                return false;
            }
            // Compiler reports these static RAM allocation
            #if ARDUINO_JSON_VERSION == 6
            if (_message_doc.capacity() < JSON_TALKIE_BUFFER_SIZE) {  // Absolute minimum
                Serial.println(F("CRITICAL: Insufficient RAM"));
                return false;
            }
            #else
            if (_message_doc.overflowed()) {
                Serial.println(F("CRITICAL: Insufficient RAM"));
                return false;
            }
            #endif
            BroadcastSocket::setCallback(listenCallback);
            _running = true;
            return true;
        }

        static void end() {
            _running = false;
            broadcast_socket.close();
        }

        static bool talk(JsonObject message) {
            if (!_running)
                return false;

            size_t size = 0;

            // In order to release memory when done
            {
                // Directly nest the editable message under "m"
                if (message.isNull()) {
                    Serial.println(F("Error: Null message received"));
                    return false;
                }

                // Set default 'id' field if missing
                if (!message.containsKey("i")) {
                    message["i"] = generateMessageId();
                }
                message["f"] = Manifesto::talk()->name;
                valid_checksum(message);

                size = serializeJson(message, _buffer, JSON_TALKIE_BUFFER_SIZE);
                if (size == 0) {
                    Serial.println(F("Error: Serialization failed"));
                } else {
                    if (message["m"] != 6) {    // echo
                        strncpy(_sent_message_id, message["i"], sizeof(_sent_message_id) - 1); // Explicit copy
                        _sent_message_id[sizeof(_sent_message_id) - 1] = '\0'; // Ensure null-termination
                    }

                    #ifdef JSONTALKIE_DEBUG
                    Serial.print("T: ");
                    serializeJson(message, Serial);
                    Serial.println();  // optional: just to add a newline after the JSON
                    #endif

                    return broadcast_socket.send(_buffer, size);
                }
            }
            return false;
        }
    };

    // Compiler reports these static RAM allocation
    #if ARDUINO_JSON_VERSION == 6
    static StaticJsonDocument<JSON_TALKIE_BUFFER_SIZE> Talker::_message_doc;
    #else
    static JsonDocument Talker::_message_doc;
    #endif
    char Talker::_buffer[JSON_TALKIE_BUFFER_SIZE] = {'\0'};
    char Talker::_sent_message_id[9] = {'\0'};  // 8 chars + null terminator
    bool Talker::_running = false;

}

// WARNING: This declares as a function: JsonTalkie::Talker json_talkie();
JsonTalkie::Talker json_talkie;

#endif
