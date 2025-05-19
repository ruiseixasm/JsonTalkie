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
#include "sockets/BroadcastSocket_UIP.hpp"
// #include "JsonTalkie.hpp"
#include "dummies/JsonTalkie_Dummy.hpp"


// // Linux testing commands:
// echo "BROADCAST 255" | nc -ubv 255.255.255.255 5005
// echo "BROADCAST 192" | nc -ubv 192.168.31.255 5005
// echo "UNICAST" | nc -ubv 192.168.31.100 5005


auto& broadcast_socket = BroadcastSocket_UIP::instance();

// Configuration
// #define USE_DHCP  // Comment out to use static IP

// Network Settings
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 31, 101);       // Arduino IP
IPAddress subnet(255, 255, 255, 0);   // Network mask
IPAddress gateway(192, 168, 31, 77);    // Router IP (if needed)
const unsigned int PORT = 5005;

EthernetUDP udp;


#ifdef JSON_TALKIE_DUMMY_HPP

JsonTalkie_Dummy json_talkie;

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    delay(2000);    // Just to give some time to Serial

    // Configure IP (static only when USE_DHCP is undefined)
    #ifndef USE_DHCP
    #endif

    Serial.print("\n\nConnecting");

    // Initialize Ethernet with static IP
    Ethernet.begin(mac, ip, gateway, subnet);

    // Start UDP
    if (udp.begin(PORT)) {
        Serial.print("\n\nUDP active on ");
        Serial.println(Ethernet.localIP());
    } else {
        Serial.println("UDP failed!");
    }
    
    Serial.println("\n\nOpening the Socket...");
    
    broadcast_socket.set_port(PORT);
    broadcast_socket.set_udp(&udp);


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


#else

JsonTalkie json_talkie;


// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
JsonTalkie::Device device = {
    "Mega", "I do a 500ms buzz!"
};

bool buzz(JsonObject json_message);
bool led_on(JsonObject json_message);
bool led_off(JsonObject json_message);
JsonTalkie::Run runCommands[] = {
    {"buzz", "Triggers buzzing", buzz},
    {"on", "Turns led On", led_on},
    {"off", "Turns led Off", led_off}
};

bool set_duration(JsonObject json_message, long duration);
JsonTalkie::Set setCommands[] = {
    // {"duration", "Sets duration", set_duration}
};

long get_total_runs(JsonObject json_message);
long get_duration(JsonObject json_message);
JsonTalkie::Get getCommands[] = {
    {"total_runs", "Gets the total number of runs", get_total_runs}
    // {"duration", "Gets duration", get_duration}
};

bool process_response(JsonObject json_message);


// MANIFESTO DECLARATION

JsonTalkie::Manifesto manifesto(
    &device,
    runCommands, sizeof(runCommands)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get),
    process_response, nullptr
);

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
    
    // Configure IP (static only when USE_DHCP is undefined)
    #ifndef USE_DHCP
    #endif

    Serial.print("\n\nConnecting");
    
    // Initialize Ethernet with static IP
    Ethernet.begin(mac, ip, gateway, subnet);

    // Start UDP
    if (udp.begin(PORT)) {
        Serial.print("\n\nUDP active on ");
        Serial.println(Ethernet.localIP());
    } else {
        Serial.println("UDP failed!");
    }
    
    Serial.println("\n\nOpening the Socket...");
    
    // By default is already 5005
    broadcast_socket.set_port(5005);
    broadcast_socket.set_udp(&udp);

    json_talkie.set_manifesto(&manifesto);
    json_talkie.plug_socket(&broadcast_socket);


    Serial.println("Talker ready");

    #ifndef BROADCAST_SOCKET_SERIAL_HPP
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
    #ifndef BROADCASTSOCKET_SERIAL
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    total_runs++;
    return true;
}


bool is_led_on = false;  // keep track of state yourself, by default it's off

bool led_on(JsonObject json_message) {
    if (!is_led_on) {
        digitalWrite(LED_BUILTIN, HIGH);
        is_led_on = true;
        total_runs++;
    } else {
        json_message["r"] = "Already On!";
        json_talkie.talk(json_message);
        return false;
    }
    return true;
}

bool led_off(JsonObject json_message) {
    if (is_led_on) {
        digitalWrite(LED_BUILTIN, LOW);
        is_led_on = false;
        total_runs++;
    } else {
        json_message["r"] = "Already Off!";
        json_talkie.talk(json_message);
        return false;
    }
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


#endif
