/**
 * @file    ESP32TestPair_Broadcast_SPI_2xESP_4MHz_Slave.ino
 * @author  Rui Seixas Monteiro
 * @brief   This is an ESP32 SPI Slave testing sketch.
 *
 * This sketch is intended to test the socket `S_Broadcast_SPI_2xESP_4MHz_Slave` that is targeted for the
 * ESP32 board as SPI Slave to be controlled by an ESP32 board as SPI Master.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - One ESP32 board as SPI Slave plus one ESP32 board as SPI Master (check CAUTION bellow)
 * 
 * CAUTION:
 * - With this sketch you can have more than one single board as SPI Slave, this happens because the
 *   SPI Sockets are broadcast sockets that send messages in Broadcast mode. So, if you choose to work
 *   with multiple ESP32 boards as SPI Slaves, make sure you connect a resistor of **around 500 Ohms** to
 *   each SPI Slave MISO pin, in the case of the ESP32 is the pin 12 or 19 for HSPI or VSPI respectively!
 * 
 *   [1st Slave ESP32 MISO] ----[500Ω]----┐
 *   [2nd Slave ESP32 MISO] ----[500Ω]----┼---- [Master ESP32 MISO]
 *   [3rd Slave ESP32 MISO] ----[500Ω]----┘
 *
 * Sockets:
 * - S_Broadcast_SPI_2xESP_4MHz_Slave
 * 
 * Manifestos:
 * - M_LedManifesto
 * 
 * Created: 2026-02-09
 */


// #define SKETCH_DEBUG

// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

#include <JsonTalkie.hpp>
#include "S_Broadcast_SPI_2xESP_4MHz_Slave.hpp"
#include "M_LedManifesto.hpp"


// TALKERS 

// Talker (led)
const char l_led_name[] = "slave";
const char l_led_desc[] = "I'm the SPI Slave";
M_LedManifesto led_manifesto(LED_BUILTIN);
JsonTalker l_led = JsonTalker(l_led_name, l_led_desc, &led_manifesto);


// SOCKETS
// Singleton requires the & (to get a reference variable)
auto& spi_socket = S_Broadcast_SPI_2xESP_4MHz_Slave::instance(VSPI_HOST);


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &spi_socket };
JsonTalker* downlinked_talkers[] = { &l_led };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);



void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(1000); // Important: Give time for serial to initialize

	#ifdef SKETCH_DEBUG
        Serial.println("\n\n=== ESP32 with Broadcast SPI STARTING ===");
    #endif

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    // Setting up broadcast sockets

	#ifdef SKETCH_DEBUG
        Serial.println("Setting up broadcast sockets...");
    #endif

	// ================== INITIALIZE VSPI ==================
	// Initialize SPI with VSPI pins: MOSI=23, MISO=19, SCK=18, CS=5
    spi_socket.begin(23, 19, 18, 5);	// MOSI, MISO, SCK, CS
	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too

    // Finally, sets the blue led as always LOW signalling this way to be a SPI Slave
    digitalWrite(LED_BUILTIN, LOW);

	#ifdef SKETCH_DEBUG
        Serial.println("Setup completed - Ready for JSON communication!");
    #endif
}


void loop() {
	message_repeater.loop();	// Keep calling the Repeater
}

