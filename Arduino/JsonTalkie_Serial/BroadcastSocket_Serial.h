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
#ifndef BROADCAST_SOCKET_SERIAL_H
#define BROADCAST_SOCKET_SERIAL_H

#include "BroadcastSocket.h"
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!

class BroadcastSocket_Serial : public BroadcastSocket {
public:
    BroadcastSocket_Serial(long baud = 9600);
    
    bool begin() override;
    void end() override;
    bool write(const uint8_t* data, size_t length) override;
    bool available() override;
    size_t read(uint8_t* buffer, size_t size) override;

private:
    long baudRate;
};

#endif // BROADCAST_SOCKET_SERIAL_H
