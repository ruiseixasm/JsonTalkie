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

// #define USE_WIFI

#define SOCKET_SERIAL 1
#define SOCKET_UDP 2
#define SOCKET_ETHERCARD 3
#define SOCKET_DUMMY 0

#ifdef USE_WIFI
#include <ESP8266WiFi.h>
#include "secrets/wifi_credentials.h"   // Make sure "secrets/" is in the gitignore before staging and pushing!!
#endif

// Choose Broadcast Socket here ---vvv
#define BROADCAST_SOCKET SOCKET_ETHERCARD

#if BROADCAST_SOCKET == SOCKET_SERIAL
    #include "sockets/BroadcastSocket_Serial.hpp"
    BroadcastSocket_Serial broadcast_socket;
#elif BROADCAST_SOCKET == SOCKET_UDP
    #include "sockets/BroadcastSocket_UDP.hpp"
    // Network configuration
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    BroadcastSocket_UDP broadcast_socket(5005);  // Port set in constructor
#elif BROADCAST_SOCKET == SOCKET_ETHERCARD
    #include "sockets/BroadcastSocket_EtherCard.hpp"
    BroadcastSocket_EtherCard* BroadcastSocket_EtherCard::_instance = nullptr;
    uint8_t Ethernet::buffer[ETHER_BUFFER_SIZE]; // Essential for EtherCard
    uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
    const uint8_t CS_PIN = 8;
    // MAC and CS pin in constructor
    // SS is a macro variable normally equal to 10
    // Change 'SS' to your Slave Select pin, if you arn't using the default pin
    BroadcastSocket_EtherCard broadcast_socket(5005, mymac, SS);
#else
    #include "sockets/BroadcastSocket_Dummy.hpp"
    BroadcastSocket_Dummy broadcast_socket;
#endif

#include "JsonTalkie.hpp"
JsonTalkie::Talker json_talkie(&broadcast_socket);


// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
const JsonTalkie::Device JsonTalkie::Manifesto::device = {
    "Buzzer", "I do a 500ms buzz!"
};

bool buzz(JsonObject json_message);
bool led_on(JsonObject json_message);
bool led_off(JsonObject json_message);
const JsonTalkie::Run JsonTalkie::Manifesto::runCommands[] = {
    {"buzz", "Triggers buzzing", buzz},
    {"on", "Turns led On", led_on},
    {"off", "Turns led Off", led_off}
};
const size_t JsonTalkie::Manifesto::runSize = sizeof(JsonTalkie::Manifesto::runCommands) / sizeof(JsonTalkie::Run);

bool set_duration(JsonObject json_message, const char* duration);
const JsonTalkie::Set JsonTalkie::Manifesto::setCommands[] = {
    // {"duration", "Sets duration", set_duration}
};
const size_t JsonTalkie::Manifesto::setSize = sizeof(JsonTalkie::Manifesto::setCommands) / sizeof(JsonTalkie::Set);

int get_duration(JsonObject json_message);
const JsonTalkie::Get JsonTalkie::Manifesto::getCommands[] = {
    // {"duration", "Gets duration", get_duration}
};
const size_t JsonTalkie::Manifesto::getSize = sizeof(JsonTalkie::Manifesto::getCommands) / sizeof(JsonTalkie::Get);

bool process_response(JsonObject json_message);
bool (*JsonTalkie::Manifesto::echo)(JsonObject) = process_response;

// END OF MANIFESTO


// Buzzer pin
#define buzzer_pin 3

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    Serial.println();
    Serial.println("Beginning Talker...");
    if (!json_talkie.begin()) {
        Serial.println("Failed to initialize Talker!");
        while(1);
    }

    #ifdef USE_WIFI

    // Connect to WiFi
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Connection successful
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    #endif


    Serial.println("Talker ready");

    #if BROADCAST_SOCKET != SOCKET_SERIAL
    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, HIGH);
    delay(10); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Sending JSON...");

    // Lives until end of function
    #if ARDUINO_JSON_VERSION == 6
    StaticJsonDocument<JSON_TALKIE_SIZE> message_doc;
    if (message_doc.capacity() == 0) {
        Serial.println("Failed to allocate JSON message_doc");
        return 0;
    }
    #else
    JsonDocument message_doc;
    if (message_doc.overflowed()) {
        Serial.println("Failed to allocate JSON message_doc");
        return 0;
    }
    #endif

    JsonObject message = message_doc.to<JsonObject>();
    message["m"] = "talk";
    json_talkie.talk(message);
}

void loop() {
    json_talkie.listen();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 39000) {

        // Lives until end of function
        #if ARDUINO_JSON_VERSION == 6
        StaticJsonDocument<JSON_TALKIE_SIZE> message_doc;
        if (message_doc.capacity() == 0) {
            Serial.println("Failed to allocate JSON message_doc");
            return 0;
        }
        #else
        JsonDocument message_doc;
        if (message_doc.overflowed()) {
            Serial.println("Failed to allocate JSON message_doc");
            return 0;
        }
        #endif
    
        JsonObject message = message_doc.to<JsonObject>();
        message["m"] = "talk";
        json_talkie.talk(message);
        lastSend = millis();
    }
}


int _duration = 5;  // Example variable

// Command implementations
bool buzz(JsonObject json_message) {
    #if BROADCAST_SOCKET != SOCKET_SERIAL
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    return true;
}

bool led_on(JsonObject json_message) {
    digitalWrite(LED_BUILTIN, HIGH);
    return true;
}

bool led_off(JsonObject json_message) {
    digitalWrite(LED_BUILTIN, LOW);
    return true;
}


bool set_duration(JsonObject json_message, int duration) {
    _duration = duration;
    return true;
}

int get_duration(JsonObject json_message) {
    return _duration;
}



bool process_response(JsonObject json_message) {
    Serial.println(json_message["r"].as<String>()); // The magic fix
    return false;
}
