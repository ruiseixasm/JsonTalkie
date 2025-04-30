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
#ifndef BROADCAST_SOCKET_H
#define BROADCAST_SOCKET_H

// Arduino-compatible type definitions
typedef unsigned char uint8_t;
typedef unsigned int size_t;

class BroadcastSocket {
public:
    virtual ~BroadcastSocket() {}
    
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual bool write(const uint8_t* data, size_t length) = 0;
    virtual bool available() = 0;
    virtual size_t read(uint8_t* buffer, size_t size) = 0;
};


#endif
