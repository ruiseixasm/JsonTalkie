/**
 * @file    ArduinoTestPair_Broadcast_SPI_Arduino_Slave.ino
 * @author  Rui Seixas Monteiro
 * @brief   This is an Arduino SPI Slave testing sketch.
 *
 * This sketch is intended to test the socket `S_Broadcast_SPI_Arduino_Slave` that is targeted for the
 * Arduino board as SPI Slave to receive messages from a SPI paired Arduino SPI Master.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - One Arduino board as SPI Slave plus one more Arduino board as SPI Master (check CAUTION bellow)
 * 
 * CAUTION:
 * - With this sketch you can have more than one single board as SPI Slave, this happens because the
 *   SPI Sockets are broadcast sockets that send messages in Broadcast mode. So, if you choose to work
 *   with multiple Arduino boards as SPI Slaves, make sure you connect a resistor of around 500 Ohms to
 *   each SPI Slave MISO pin, in the case of the Arduino Nano and Uno is the pin 12!
 * 
 *   [1st Slave Arduino MISO] ----[500Ω]----┐
 *   [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master Arduino MISO]
 *   [3rd Slave Arduino MISO] ----[500Ω]----┘
 *
 * Sockets:
 * - S_Broadcast_SPI_Arduino_Slave
 * 
 * Manifestos:
 * - M_LedManifesto
 * 
 * Created: 2026-02-09
 */

// #define SKETCH_DEBUG

#include <JsonTalkie.hpp>
#include "S_Broadcast_SPI_Arduino_Slave.h"
#include "M_LedManifesto.hpp"


#define YELLOW_LED_PIN 2	// External BUZZER pin

// TALKERS 

// Talker (led)
const char l_led_name[] = "slave";
const char l_led_desc[] = "I'm the SPI Slave";
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

