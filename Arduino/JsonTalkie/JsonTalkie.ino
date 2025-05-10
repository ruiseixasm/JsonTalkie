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
#define USE_STATIC_IP

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

// Network settings
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};      // MAC address
byte my_ip[] = {192, 168, 31, 100};                     // Arduino IP
byte gw_ip[] = {192, 168, 31, 77};                      // IP of the main router, gateway
byte dns_ip[] = {192, 168, 31, 77};                     // DNS address is the same as the gateway router
byte mask[] = {255, 255, 255, 0};                       // NEEDED FOR NETWORK BROADCAST
const uint16_t PORT = 5005;                             // UDP port

#if BROADCAST_SOCKET == SOCKET_SERIAL
    #include "sockets/BroadcastSocket_Serial.hpp"
#elif BROADCAST_SOCKET == SOCKET_UDP
    #include "sockets/BroadcastSocket_UDP.hpp"
#elif BROADCAST_SOCKET == SOCKET_ETHERCARD
    #define BUFFER_SIZE 256
    #include "sockets/BroadcastSocket_EtherCard.hpp"
    byte Ethernet::buffer[BUFFER_SIZE];  // Ethernet buffer
#else
    #include "sockets/BroadcastSocket_Dummy.hpp"
#endif

#include "JsonTalkie.hpp"


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

long get_total_runs(JsonObject json_message);
long get_duration(JsonObject json_message);
const JsonTalkie::Get JsonTalkie::Manifesto::getCommands[] = {
    {"total_runs", "Gets the total number of runs", get_total_runs}
    // {"duration", "Gets duration", get_duration}
};
const size_t JsonTalkie::Manifesto::getSize = sizeof(JsonTalkie::Manifesto::getCommands) / sizeof(JsonTalkie::Get);

bool process_response(JsonObject json_message);
bool (*JsonTalkie::Manifesto::echo)(JsonObject) = process_response;
bool (*JsonTalkie::Manifesto::error)(JsonObject) = nullptr;

// END OF MANIFESTO


// Buzzer pin
#define buzzer_pin 3

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    // Saving string in PROGMEM (flash) to save RAM memory
    Serial.println("\n\nOpening the Socket...");
    
    // MAC and CS pin in constructor
    // SS is a macro variable normally equal to 10
    if (!ether.begin(BUFFER_SIZE, mac, SS)) {
        Serial.println("Failed to access ENC28J60");
        while (1);
    }
    // Set static IP (disable DHCP)
    if (!ether.staticSetup(my_ip, gw_ip, dns_ip, mask)) {
        Serial.println("Failed to access ENC28J60");
        while (1);
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
}

void loop() {
    json_talkie.listen();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 39000) {

        // Lives until end of function
        #if ARDUINO_JSON_VERSION == 6
        StaticJsonDocument<BROADCAST_SOCKET_BUFFER_SIZE> message_doc;
        if (message_doc.capacity() < BROADCAST_SOCKET_BUFFER_SIZE) {  // Absolute minimum
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie.talk(message);
        }
        #else
        JsonDocument message_doc;
        if (message_doc.overflowed()) {
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie.talk(message);
        }
        #endif
    
        lastSend = millis();
    }
}


long total_runs = 0;
long _duration = 5;  // Example variable

// Command implementations
bool buzz(JsonObject json_message) {
    #if BROADCAST_SOCKET != SOCKET_SERIAL
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    total_runs++;
    return true;
}

bool led_on(JsonObject json_message) {
    digitalWrite(LED_BUILTIN, HIGH);
    total_runs++;
    return true;
}

bool led_off(JsonObject json_message) {
    digitalWrite(LED_BUILTIN, LOW);
    total_runs++;
    return true;
}


bool set_duration(JsonObject json_message, long duration) {
    _duration = duration;
    return true;
}

long get_duration(JsonObject json_message) {
    return _duration;
}

long get_total_runs(JsonObject json_message) {
    return total_runs;
}


bool process_response(JsonObject json_message) {
    Serial.print(json_message["f"].as<String>());
    Serial.print(" - ");
    if (json_message.containsKey("r")) {
        Serial.println(json_message["r"].as<String>());
    } else if (json_message.containsKey("d")) {
        Serial.println(json_message["d"].as<String>());
    } else {
        Serial.println("Empty echo received!");
    }
    return false;
}
