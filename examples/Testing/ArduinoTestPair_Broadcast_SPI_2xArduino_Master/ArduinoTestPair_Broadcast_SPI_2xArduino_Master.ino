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

#define BUZZER_PIN 2  // Fallback definition if not already defined


#include <JsonTalkie.hpp>
// ONLY THE CHANGED LIBRARY ALLOWS THE RECEPTION OF BROADCASTED UDP PACKAGES TO 255.255.255.255
#include "S_SocketSerial.hpp"
#include "S_Broadcast_SPI_2xArduino_Master.hpp"
#include "M_SPIMasterManifesto.hpp"


// TALKERS 

// Duo Talker
const char t_duo_name[] = "master";
const char t_duo_desc[] = "I'm the SPI Master";
M_SPIMasterManifesto duo_manifesto;
JsonTalker t_duo = JsonTalker(t_duo_name, t_duo_desc, &duo_manifesto);


// SOCKETS
// Singleton requires the & (to get a reference variable)
auto& serial_socket = S_SocketSerial::instance();
/**
 * CAUTION:
 * - For more than one single board as SPI Slave, make sure you connect a resistor of around 500 Ohms to
 * each SPI Slave MISO pin, in the case of the Arduino Nano and Uno is the pin 12!
 * 
 * [1st Slave Arduino MISO] ----[500Ω]----┐
 * [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master Arduino MISO]
 * [3rd Slave Arduino MISO] ----[500Ω]----┘
 *
 */
const int spi_pins[] = {7, SS, 8};
auto& spi_socket = S_Broadcast_SPI_2xArduino_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &serial_socket };
JsonTalker* downlinked_talkers[] = { &t_duo };
BroadcastSocket* downlinked_sockets[] = { &spi_socket };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*),
		downlinked_sockets, sizeof(downlinked_sockets)/sizeof(BroadcastSocket*)
	);



void setup() {
    // Initialize pins FIRST before anything else
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(2000); // Important: Give time for serial to initialize

	#ifdef SKETCH_DEBUG
        Serial.println("\n\n=== ESP32 with Broadcast SPI STARTING ===");
    #endif
    
    // Add a small LED blink to confirm code is running
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Setting up broadcast sockets

	#ifdef SKETCH_DEBUG
        Serial.println("Setting up broadcast sockets...");
    #endif

	#ifdef SKETCH_DEBUG
        Serial.println("Setup completed - Ready for JSON communication!");
    #endif
}


void loop() {
	message_repeater.loop();	// Keep calling the Repeater
}

