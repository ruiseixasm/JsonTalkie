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
#define BROADCAST_SOCKET_BUFFER_SIZE 128

// To occupy less Flash memory
#define ARDUINO_JSON_VERSION 6

// #define JSONTALKIE_DEBUG


// Keys:
//     c: checksum
//     d: description
//     e: error code
//     f: from
//     g: echo roger code
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
//     7 error

// Echo codes (g):
//     0 - ROGER
//     1 - UNKNOWN
//     2 - NONE

// Error types (e):
//     0 - Unknown sender
//     1 - Message missing the checksum
//     2 - Message corrupted
//     3 - Wrong message code
//     4 - Message NOT identified
//     5 - Set command arrived too late

class JsonTalkie {
public:

    // JSONTALKIE DEFINITIONS


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
        bool (*function)(JsonObject, long);  // Function pointer (long)
    };

    struct Get {
        const char* PROGMEM name;      // "buzz", "print", etc.
        const char* PROGMEM desc;      // Description
        long (*function)(JsonObject);  // Function pointer (no args)
    };

    // Manifesto Structure Definition
    struct Manifesto {

        Device* device = nullptr;
        Run* runCommands = nullptr;
        size_t runSize = 0;
        Set* setCommands = nullptr;
        size_t setSize = 0;
        Get* getCommands = nullptr;
        size_t getSize = 0;
        bool (*echo)(JsonObject) = nullptr;
        bool (*error)(JsonObject) = nullptr;

        Run* get_run(const char* cmd) {
            for (int index = 0; index < runSize; ++index) {
                if (strcmp(cmd, runCommands[index].name) == 0) {
                    return &runCommands[index];  // Returns the function
                }
            }
            return nullptr;
        }
    
        Set* get_set(const char* cmd) {
            for (int index = 0; index < setSize; ++index) {
                if (strcmp(cmd, setCommands[index].name) == 0) {
                    return &setCommands[index];  // Returns the function
                }
            }
            return nullptr;
        }
    
        Get* get_get(const char* cmd) {
            for (int index = 0; index < getSize; ++index) {
                if (strcmp(cmd, getCommands[index].name) == 0) {
                    return &getCommands[index];  // Returns the function
                }
            }
            return nullptr;
        }

        // Add this constructor, because Manifesto struct has methods,
        //    so it's not considered an aggregate, and therefore cannot be initialized using a brace-enclosed list like this.
        Manifesto(Device* d, Run* r, size_t rsz, Set* s, size_t ssz, 
                Get* g, size_t gsz, bool (*e)(JsonObject), bool (*err)(JsonObject))
            : device(d),
            runCommands(r), runSize(rsz),
            setCommands(s), setSize(ssz),
            getCommands(g), getSize(gsz),
            echo(e), error(err) {}
    };



private:
    // Configuration parameters
    BroadcastSocket* _socket = nullptr;
    Manifesto* _manifesto = nullptr;


    // Compiler reports these static RAM allocation
    #if ARDUINO_JSON_VERSION == 6
    StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> _message_doc;
    #else
    JsonDocument _message_doc;
    #endif
    char _buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};
    uint32_t _sent_set_time[2] = {0};   // Keeps two time stamp
    String _set_name = "";              // Keeps the device name
    bool _check_set_time = false;

public:
    void set_manifesto(Manifesto* manifesto) {
        _manifesto = manifesto;
    }

    Manifesto* get_manifesto() {
        return _manifesto;
    }

    void plug_socket(BroadcastSocket* socket) {
        _socket = socket;
    }

    void unplug_socket() {
        _socket = nullptr;
    }



    bool talk(JsonObject message, bool as_reply = false) {
        if (_socket == nullptr || _manifesto == nullptr || _manifesto->device == nullptr) return false;
        // In order to release memory when done
        {
            // Directly nest the editable message under "m"
            if (message.isNull()) {
                #ifdef JSONTALKIE_DEBUG
                Serial.println(F("Error: Null message received"));
                #endif
                return false;
            }

            // Set default 'id' field if missing
            if (!message.containsKey("i")) {
                message["i"] = generateMessageId();
            }
            message["f"] = _manifesto->device->name;
            valid_checksum(message);

            size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
            if (len == 0) {
                #ifdef JSONTALKIE_DEBUG
                Serial.println(F("Error: Serialization failed"));
                #endif
            } else {
                
                #ifdef JSONTALKIE_DEBUG
                Serial.print(F("T: "));
                serializeJson(message, Serial);
                Serial.println();  // optional: just to add a newline after the JSON
                #endif

                return _socket->send(_buffer, len, as_reply);
            }
        }
        return false;
    }


    void listen() {
        if (_socket == nullptr) return;
        size_t len = _socket->receive(_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        if (len > 0) {

            #ifdef JSONTALKIE_DEBUG
            Serial.print(F("L: "));
            Serial.write(_buffer, len);  // Properly prints raw bytes as characters
            Serial.println();            // Adds newline after the printed data
            #endif

            DeserializationError error = deserializeJson(_message_doc, _buffer);
            if (error) {
                #ifdef JSONTALKIE_DEBUG
                Serial.println(F("Failed to deserialize buffer"));
                #endif
                return;
            }
            JsonObject message = _message_doc.as<JsonObject>();

            if (validateTalk(message)) {

                #ifdef JSONTALKIE_DEBUG
                Serial.print(F("Listened: "));
                serializeJson(message, Serial);
                Serial.println();  // optional: just to add a newline after the JSON
                #endif

                // Only set messages are time checked
                if (message["m"].as<int>() == 3) {  // 3 - set
                    _sent_set_time[0] = message["i"].as<uint32_t>();
                    _sent_set_time[1] = generateMessageId();
                    _set_name = message["f"].as<String>(); // Explicit conversion
                    _check_set_time = true;
                }

                process(message);
            }
        // In theory, a UDP packet on a local area network (LAN) could survive
        // for about 4.25 minutes (255 seconds).
        } else if (_check_set_time && millis() - _sent_set_time[1] > 255 * 1000) {
            _check_set_time = false;
        }
    }


private:
    static uint32_t generateMessageId() {
        // Generates a 32-bit wrapped timestamp ID using overflow.
        return (uint32_t)millis();  // millis() is already an unit32_t (unsigned long int) data return
    }


    bool valid_checksum(JsonObject message) {
        // Use a static buffer size, large enough for your JSON
        uint16_t message_checksum = 0;
        if (message.containsKey("c")) {
            message_checksum = message["c"].as<uint16_t>();
        }
        message["c"] = 0;
        size_t len = serializeJson(message, _buffer, BROADCAST_SOCKET_BUFFER_SIZE);
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


    bool validateTalk(JsonObject message) {
        if (_manifesto == nullptr || _manifesto->device == nullptr) return false;
        #ifdef JSONTALKIE_DEBUG
        Serial.println(F("Validating..."));
        #endif
        
        // Error types:
        //     0 - Unknown sender
        //     1 - Message missing the checksum
        //     2 - Message corrupted
        //     3 - Wrong message code
        //     4 - Message NOT identified
        //     5 - Set command arrived too late

        if (!(message["m"].as<int>() == 0 || message["m"].as<int>() == 5
                || message.containsKey("t") && (message["t"] == _manifesto->device->name || message["t"] == "*"))) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println(F("Message NOT for me!"));
            #endif
            return false;
        }
        if (!(message.containsKey("f") && message["f"].is<String>())) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println(0);
            #endif
            message["m"] = 7;   // error
            message["t"] = "*";
            message["f"] = _manifesto->device->name;
            message["e"] = 0;
            talk(message);
            return false;
        }
        if (!(message.containsKey("c") && message["c"].is<uint16_t>())) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println(1);
            #endif
            message["m"] = 7;   // error
            message["t"] = message["f"];
            message["f"] = _manifesto->device->name;
            message["e"] = 1;
            talk(message, true);
            return false;
        }
        if (!valid_checksum(message)) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println(2);
            #endif
            message["m"] = 7;   // error
            message["t"] = message["f"];
            message["f"] = _manifesto->device->name;
            message["e"] = 2;
            talk(message, true);
            return false;
        }
        if (!(message.containsKey("m") && message["m"].is<int>())) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println(3);
            #endif
            message["m"] = 7;   // error
            message["t"] = message["f"];
            message["f"] = _manifesto->device->name;
            message["e"] = 3;
            talk(message, true);
            return false;
        }
        if (!(message.containsKey("i") && message["i"].is<uint32_t>())) {
            #ifdef JSONTALKIE_DEBUG
            Serial.println(4);
            #endif
            message["m"] = 7;   // error
            message["t"] = message["f"];
            message["f"] = _manifesto->device->name;
            message["e"] = 4;
            talk(message, true);
            return false;
        }
        // Only set messages are time checked
        // In theory, a UDP packet on a local area network (LAN) could survive for about 4.25 minutes (255 seconds).
        if (_check_set_time && message["m"].as<int>() == 3 && message["f"].as<String>() == _set_name) {   // 3 - set
            uint32_t delta = _sent_set_time[0] - message["i"].as<uint32_t>();
            if (delta < 255 && delta != 0) {
                #ifdef JSONTALKIE_DEBUG
                Serial.println(5);
                #endif
                message["m"] = 7;   // error
                message["t"] = message["f"];
                message["f"] = _manifesto->device->name;
                message["e"] = 5;
                talk(message, true);
                return false;
            }
        }
        // NEEDS TO BE COMPLETED
        #ifdef JSONTALKIE_DEBUG
        Serial.println(F("Validated"));
        #endif
        return true;
    }
    

    bool process(JsonObject message) {
        if (_manifesto == nullptr || _manifesto->device == nullptr) return false;
        // Echo codes:
        //     0 - ROGER
        //     1 - UNKNOWN
        //     2 - NONE

        #ifdef JSONTALKIE_DEBUG
        Serial.print(F("Process: "));
        serializeJson(message, Serial);
        Serial.println();  // optional: just to add a newline after the JSON
        #endif

        int message_code = message["m"].as<int>(); // Throws on type mismatch
        message["t"] = message["f"];
        message["m"] = 6;
        if (message_code == 0) {            // talk
            message["w"] = 0;
            message["d"] = _manifesto->device->desc;
            return talk(message, true);
        } else if (message_code == 1) {     // list
            bool none_list = true;
            message["w"] = 2;
            for (size_t run_i = 0; run_i < _manifesto->runSize; ++run_i) {
                message["n"] = _manifesto->runCommands[run_i].name;
                message["d"] = _manifesto->runCommands[run_i].desc;
                none_list = false;
                talk(message, true);
            }
            message["w"] = 3;
            for (size_t set_i = 0; set_i < _manifesto->setSize; ++set_i) {
                message["n"] = _manifesto->setCommands[set_i].name;
                message["d"] = _manifesto->setCommands[set_i].desc;
                none_list = false;
                talk(message, true);
            }
            message["w"] = 4;
            for (size_t get_i = 0; get_i < _manifesto->getSize; ++get_i) {
                message["n"] = _manifesto->getCommands[get_i].name;
                message["d"] = _manifesto->getCommands[get_i].desc;
                none_list = false;
                talk(message, true);
            }
            if(none_list) {
                message["g"] = 2;       // NONE
            }
            return true;
        } else if (message_code == 2) {     // run
            message["w"] = 2;
            if (message.containsKey("n")) {
                const Run* run = _manifesto->get_run(message["n"]);
                if (run == nullptr) {
                    message["g"] = 1;   // UNKNOWN
                    talk(message, true);
                    return false;
                }
                message["g"] = 0;       // ROGER
                talk(message, true);
                run->function(message);
                return true;
            }
        } else if (message_code == 3) {     // set
            message["w"] = 3;
            if (message.containsKey("n") && message.containsKey("v") && message["v"].is<long>()) {
                const Set* set = _manifesto->get_set(message["n"]);
                if (set == nullptr) {
                    message["g"] = 1;   // UNKNOWN
                } else {
                    message["g"] = 0;   // ROGER
                }
                talk(message, true);
                if (set != nullptr) {
                    set->function(message, message["v"].as<long>());
                }
                return true;
            }
        } else if (message_code == 4) {     // get
            message["w"] = 4;
            if (message.containsKey("n")) {
                message["w"] = message_code;
                const Get* get = _manifesto->get_get(message["n"]);
                if (get == nullptr) {
                    message["g"] = 1;   // UNKNOWN
                } else {
                    message["g"] = 0;   // ROGER
                    message["v"] = get->function(message);
                }
                return talk(message, true);
            }
        } else if (message_code == 5) {     // sys
            message["w"] = 5;
            
            // AVR Boards (Uno, Nano, Mega) - Check RAM size
            #ifdef __AVR__
            uint16_t ramSize = RAMEND - RAMSTART + 1;
            if (ramSize == 2048)
                message["d"] = F("Arduino Uno/Nano (ATmega328P)");
            else if (ramSize == 8192)
                message["d"] = F("Arduino Mega (ATmega2560)");
            else
                message["d"] = F("Unknown AVR Board");
            
            // ESP8266
            #elif defined(ESP8266)
            message["d"] = "ESP8266 (Chip ID: " + String(ESP.getChipId()) + ")";
            
            // ESP32
            #elif defined(ESP32)
            message["d"] = "ESP32 (Rev: " + String(ESP.getChipRevision()) + ")";
            
            // Teensy Boards
            #elif defined(TEENSYDUINO)
                #if defined(__IMXRT1062__)
                    message["d"] = F("Teensy 4.0/4.1 (i.MX RT1062)");
                #elif defined(__MK66FX1M0__)
                    message["d"] = F("Teensy 3.6 (MK66FX1M0)");
                #elif defined(__MK64FX512__)
                    message["d"] = F("Teensy 3.5 (MK64FX512)");
                #elif defined(__MK20DX256__)
                    message["d"] = F("Teensy 3.2/3.1 (MK20DX256)");
                #elif defined(__MKL26Z64__)
                    message["d"] = F("Teensy LC (MKL26Z64)");
                #else
                    message["d"] = F("Unknown Teensy Board");
                #endif

            // ARM (Due, Zero, etc.)
            #elif defined(__arm__)
            message["d"] = F("ARM-based Board");

            // Unknown Board
            #else
            message["d"] = F("Unknown Board");

            #endif

            return talk(message, true);
        } else if (message_code == 6) {     // echo
            if (_manifesto->echo != nullptr) {
                _manifesto->echo(message);
                return true;
            }
        } else if (message_code == 7) {     // error
            if (_manifesto->error != nullptr) {
                _manifesto->error(message);
                return true;
            }
        }
        return false;
    }
};

#endif
