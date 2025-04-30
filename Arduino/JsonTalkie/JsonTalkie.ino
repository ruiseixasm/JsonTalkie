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

BroadcastSocket_Serial serialSocket(9600); // Using default Serial

void setup() {
    Serial.begin(115200); // For debug output
    while(!Serial); // Wait for serial port to connect (for USB)
    
    if (!serialSocket.begin()) {
        Serial.println("Failed to initialize serial!");
        while(1);
    }
    Serial.println("Ready for JSON communication");
}

void loop() {
    // Example: Send JSON data
    const char* jsonMsg = "{\"status\":\"ok\"}";
    if (!serialSocket.write((const uint8_t*)jsonMsg, strlen(jsonMsg))) {
        Serial.println("Send failed");
    }

    // Check for incoming data
    if (serialSocket.available()) {
        uint8_t buffer[256];
        size_t bytesRead = serialSocket.read(buffer, sizeof(buffer)-1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            Serial.print("Received JSON: ");
            Serial.println((char*)buffer);
        }
    }
    
    delay(1000);
}
