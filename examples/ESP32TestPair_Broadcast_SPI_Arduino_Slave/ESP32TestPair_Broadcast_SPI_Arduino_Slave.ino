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


// #define SKETCH_DEBUG

#include <JsonTalkie.hpp>
// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "S_Broadcast_SPI_Arduino_Slave.h"
#include "M_LedManifesto.hpp"


#define YELLOW_LED_PIN 2	// External BUZZER pin

// TALKERS 

// Talker (led)
const char l_led_name[] = "slave";
const char l_led_desc[] = "I turn led Blue on and off";
M_LedManifesto led_manifesto(YELLOW_LED_PIN);
JsonTalker l_led = JsonTalker(l_led_name, l_led_desc, &led_manifesto);


// SOCKETS
// Singleton requires the & (to get a reference variable)
auto& spi_socket = S_Broadcast_SPI_Arduino_Slave::instance();


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &spi_socket };
JsonTalker* downlinked_talkers[] = { &l_led };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);



void setup() {
    // Initialize pins FIRST before anything else
    pinMode(YELLOW_LED_PIN, OUTPUT);
    digitalWrite(YELLOW_LED_PIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(1000); // Important: Give time for serial to initialize

	#ifdef SKETCH_DEBUG
        Serial.println("\n\n=== ESP32 with Broadcast SPI STARTING ===");
    #endif

    // Add a small LED blink to confirm code is running
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(100);
    digitalWrite(YELLOW_LED_PIN, LOW);
    delay(100);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(100);
    digitalWrite(YELLOW_LED_PIN, LOW);
    
    // Setting up broadcast sockets

	#ifdef SKETCH_DEBUG
        Serial.println("Setting up broadcast sockets...");
    #endif

	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too

    // Finally, sets the blue led as always LOW signalling this way to be a SPI Slave
    digitalWrite(YELLOW_LED_PIN, LOW);

	#ifdef SKETCH_DEBUG
        Serial.println("Setup completed - Ready for JSON communication!");
    #endif
}


void loop() {
	message_repeater.loop();	// Keep calling the Repeater
}

