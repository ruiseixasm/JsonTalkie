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
#include "BroadcastSocket_Serial.hpp"
#include "JsonTalkie.hpp"


BroadcastSocket_Serial socket_serial(9600);
JsonTalkie json_talkie(&socket_serial);


// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
const Talker Manifesto::talker = {
    'Buzzer', 'This device does a 500ms buzz!'
};

const char* buzz();
const Run Manifesto::runCommands[] = {
    {"buzz", "Triggers buzzing", buzz}
};
const size_t Manifesto::runSize = sizeof(Manifesto::runCommands) / sizeof(Run);

const char* set_duration(const char* duration);
const Set Manifesto::setCommands[] = {
    {"duration", "Sets duration", set_duration}
};
const size_t Manifesto::setSize = sizeof(Manifesto::setCommands) / sizeof(Set);

const char* get_duration();
const Get Manifesto::getCommands[] = {
    {"duration", "Gets duration", get_duration}
};
const size_t Manifesto::getSize = sizeof(Manifesto::getCommands) / sizeof(Get);

bool process_response(StaticJsonDocument<256>* message, const char* response);
bool (*Manifesto::echo)(StaticJsonDocument<256>*, const char*) = process_response;

// END OF MANIFESTO



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



float _duration = 0.5f;  // Example variable

// Command implementations
const char* buzz() {
    static char buffer[32];  // Reusable buffer
    snprintf(buffer, sizeof(buffer), "Buzzed for %.1fs", _duration);
    delay(_duration * 1000);
    return buffer;
}

const char* set_duration(const char* duration) {
    _duration = String(duration).toFloat();
    static char buffer[32];  // Reusable buffer
    snprintf(buffer, sizeof(buffer), "Set duration: %ss", floatToStr(_duration));
    return buffer;
}

const char* get_duration() {
    static char buffer[32];  // Reusable buffer
    snprintf(buffer, sizeof(buffer), "Get duration: %ds", _duration);
    return buffer;
}

bool process_response(StaticJsonDocument<256>* message, const char* response) {
    Serial.println((*msg)["response"].as<String>()); // The magic fix
    return true;
}
