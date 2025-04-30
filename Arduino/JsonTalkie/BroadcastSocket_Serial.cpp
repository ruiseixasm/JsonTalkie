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
#include "BroadcastSocket_Serial.h"

BroadcastSocket_Serial::BroadcastSocket_Serial(long baud) 
    : baudRate(baud) {}

bool BroadcastSocket_Serial::begin() {
    Serial.begin(baudRate);
    return true;
}

void BroadcastSocket_Serial::end() {
    Serial.end();
}

bool BroadcastSocket_Serial::write(const uint8_t* data, size_t length) {
    return Serial.write(data, length) == length;
}

bool BroadcastSocket_Serial::available() {
    return Serial.available();
}

size_t BroadcastSocket_Serial::read(uint8_t* buffer, size_t size) {
    return Serial.readBytes((char*)buffer, size);
}
