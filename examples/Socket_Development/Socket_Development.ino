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
#include <dummies/JsonTalkie_Dummy.hpp>
#include <dummies/BroadcastSocket_Dummy.hpp>

JsonTalkie_Dummy json_talkie;
auto& broadcast_socket = BroadcastSocket_Dummy::instance();


// Network settings
#define PORT 5005                                       // UDP port


void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    // Saving string in PROGMEM (flash) to save RAM memory
    Serial.println("\n\nOpening the Socket...");
    
    // By default is already 5005
    broadcast_socket.set_port(PORT);

    json_talkie.plug_socket(&broadcast_socket);

    Serial.println("Talker ready");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Sending JSON...");
}


void loop() {
    json_talkie.listen();
    json_talkie.talk(nullptr);
}

