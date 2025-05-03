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

#define SOCKET_SERIAL 1
#define SOCKET_UDP 2
#define SOCKET_ETHERCARD 3
#define SOCKET_DUMMY 0

// Choose Broadcast Socket here ---vvv
#define BROADCAST_SOCKET SOCKET_DUMMY

#if BROADCAST_SOCKET == SOCKET_SERIAL
    #include "sockets/BroadcastSocket_Serial.hpp"
    BroadcastSocket_Serial broadcast_socket;
#elif BROADCAST_SOCKET == SOCKET_UDP
    #include "sockets/BroadcastSocket_UDP.hpp"
    BroadcastSocket_UDP broadcast_socket;
#elif BROADCAST_SOCKET == SOCKET_ETHERCARD
    #include "sockets/BroadcastSocket_EtherCard.hpp"
    BroadcastSocket_EtherCard broadcast_socket;
#else
    #include "sockets/BroadcastSocket_Dummy.hpp"
    BroadcastSocket_Dummy broadcast_socket;
#endif

#include "JsonTalkie.hpp"
JsonTalkie::Talker json_talkie(&broadcast_socket);


// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
const JsonTalkie::Device JsonTalkie::Manifesto::device = {
    "Buzzer", "This device does a 500ms buzz!"
};

const char* buzz();
const JsonTalkie::Run JsonTalkie::Manifesto::runCommands[] = {
    {"buzz", "Triggers buzzing", buzz}
};
const size_t JsonTalkie::Manifesto::runSize = sizeof(JsonTalkie::Manifesto::runCommands) / sizeof(JsonTalkie::Run);

const char* set_duration(const char* duration);
const JsonTalkie::Set JsonTalkie::Manifesto::setCommands[] = {
    // {"duration", "Sets duration", set_duration}
};
const size_t JsonTalkie::Manifesto::setSize = sizeof(JsonTalkie::Manifesto::setCommands) / sizeof(JsonTalkie::Set);

const char* get_duration();
const JsonTalkie::Get JsonTalkie::Manifesto::getCommands[] = {
    // {"duration", "Gets duration", get_duration}
};
const size_t JsonTalkie::Manifesto::getSize = sizeof(JsonTalkie::Manifesto::getCommands) / sizeof(JsonTalkie::Get);

bool process_response(StaticJsonDocument<256>* message, const char* response);
bool (*JsonTalkie::Manifesto::echo)(StaticJsonDocument<256>*, const char*) = process_response;

// END OF MANIFESTO


// Buzzer pin
#define buzzer_pin 3

void setup() {
    Serial.begin(9600);
    while (!Serial);
    
    if (!json_talkie.begin()) {
        Serial.println("Failed to initialize Talker!");
        while(1);
    }

    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, LOW);

    Serial.println("Talker ready");
    Serial.println("Sending JSON...");
    StaticJsonDocument<256> doc;
    doc["type"] = "talk";
    json_talkie.talk(doc.as<JsonObject>());
}

void loop() {
    json_talkie.listen();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 30000) {
        StaticJsonDocument<256> doc;
        doc["type"] = "talk";
        json_talkie.talk(doc.as<JsonObject>());
        lastSend = millis();
    }
}


float _duration = 0.2f;  // Example variable

// Command implementations
const char* buzz() {
    static char buffer[32];  // Reusable buffer
    snprintf(buffer, sizeof(buffer), "Buzzed for %.1fs", _duration);
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration * 1000); 
    digitalWrite(buzzer_pin, LOW);
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
    // Serial.println((*message)["response"].as<String>()); // The magic fix
    return true;
}
