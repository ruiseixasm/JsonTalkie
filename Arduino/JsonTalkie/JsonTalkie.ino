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
JsonTalkie::Talker json_talkie(&socket_serial);


// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
const JsonTalkie::Device JsonTalkie::Manifesto::device = {
    'Buzzer', 'This device does a 500ms buzz!'
};

const char* buzz();
const JsonTalkie::Run JsonTalkie::Manifesto::runCommands[] = {
    {"buzz", "Triggers buzzing", buzz}
};
const size_t JsonTalkie::Manifesto::runSize = sizeof(JsonTalkie::Manifesto::runCommands) / sizeof(JsonTalkie::Run);

const char* set_duration(const char* duration);
const JsonTalkie::Set JsonTalkie::Manifesto::setCommands[] = {
    {"duration", "Sets duration", set_duration}
};
const size_t JsonTalkie::Manifesto::setSize = sizeof(JsonTalkie::Manifesto::setCommands) / sizeof(JsonTalkie::Set);

const char* get_duration();
const JsonTalkie::Get JsonTalkie::Manifesto::getCommands[] = {
    {"duration", "Gets duration", get_duration}
};
const size_t JsonTalkie::Manifesto::getSize = sizeof(JsonTalkie::Manifesto::getCommands) / sizeof(JsonTalkie::Get);

bool process_response(StaticJsonDocument<256>* message, const char* response);
bool (*JsonTalkie::Manifesto::echo)(StaticJsonDocument<256>*, const char*) = process_response;

// END OF MANIFESTO



void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    if (!json_talkie.begin()) {
        Serial.println("Failed to initialize Talker!");
        while(1);
    }
    Serial.println("Talker ready");
}

void loop() {
    json_talkie.listen();
    
    // Example: Send a message every 5 seconds
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 5000) {
        DynamicJsonDocument doc(128);
        doc["type"] = "talk";
        json_talkie.talk(doc.as<JsonObject>());
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
    snprintf(buffer, sizeof(buffer), "Set duration: %ss", JsonTalkie::floatToStr(_duration));
    return buffer;
}

const char* get_duration() {
    static char buffer[32];  // Reusable buffer
    snprintf(buffer, sizeof(buffer), "Get duration: %ds", _duration);
    return buffer;
}

bool process_response(StaticJsonDocument<256>* message, const char* response) {
    Serial.println((*message)["response"].as<String>()); // The magic fix
    return true;
}
