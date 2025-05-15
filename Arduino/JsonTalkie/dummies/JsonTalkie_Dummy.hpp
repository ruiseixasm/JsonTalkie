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
#ifndef JSON_TALKIE_DUMMY_HPP
#define JSON_TALKIE_DUMMY_HPP

#include "BroadcastSocket.hpp"
#include <Arduino.h>


// Readjust if absolutely necessary
#define BROADCAST_SOCKET_BUFFER_SIZE 128

// #define JSONTALKIE_DEBUG


class JsonTalkie_Dummy {
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


    char _buffer[BROADCAST_SOCKET_BUFFER_SIZE] = {'\0'};

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
        
        return false;
    }


    void listen() {
        if (_socket == nullptr) return;
        size_t len = _socket->receive(_buffer, BROADCAST_SOCKET_BUFFER_SIZE);
        if (len > 0) {
            
        }
    }


};

#endif
