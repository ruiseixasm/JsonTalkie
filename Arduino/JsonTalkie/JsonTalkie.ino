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
#include "JsonTalkie.h"
#include "BroadcastSocket_Serial.h"

BroadcastSocket_Serial socket(9600);
const char* manifesto = R"(
{
    "talker": {
        "name": "Arduino1",
        "description": "Basic Arduino Talker"
    },
    "get": {
        "temperature": {
            "description": "Current temperature",
            "function": "getTemp"
        }
    }
}
)";

JsonTalkie talkie(&socket, manifesto);

float getTemperature() {
    return 25.0; // Example temperature reading
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    if (!talkie.begin()) {
        Serial.println("Failed to initialize JsonTalkie!");
        while(1);
    }
    Serial.println("JsonTalkie ready");
}

void loop() {
    talkie.listen();
    
    // Example: Send a message every 5 seconds
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 5000) {
        DynamicJsonDocument doc(128);
        doc["type"] = "talk";
        talkie.talk(doc.as<JsonObject>());
        lastSend = millis();
    }
}
