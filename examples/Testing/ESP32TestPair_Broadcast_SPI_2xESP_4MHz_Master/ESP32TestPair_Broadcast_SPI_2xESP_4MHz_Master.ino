/**
 * @file    ESP32TestPair_Broadcast_SPI_2xESP_4MHz_Master.ino
 * @author  Rui Seixas Monteiro
 * @brief   This is an ESP32 SPI Master testing sketch.
 *
 * This sketch is intended to test the socket `S_Broadcast_SPI_2xESP_4MHz_Master` that is targeted for the
 * ESP32 board as SPI Master to control one or more ESP32 boards as SPI Slaves.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - One ESP32 board as SPI Master plus one or more ESP32 boards as SPI Slaves (check CAUTION bellow)
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
 * - S_Broadcast_SPI_2xESP_4MHz_Master
 * - S_SocketSerial
 * 
 * Manifestos:
 * - M_SPIMasterManifesto
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
#include "S_Broadcast_SPI_2xESP_4MHz_Master.hpp"
#include "S_SocketSerial.hpp"
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
#define HSPI_CS 15
/**
 * CAUTION:
 * - For more than one single board as SPI Slave, make sure you connect a resistor of **around 500 Ohms** to
 *   each SPI Slave MISO pin, in the case of the ESP32 Nano and Uno is the pin 12!
 * 
 *   [1st Slave ESP32 MISO] ----[500Ω]----┐
 *   [2nd Slave ESP32 MISO] ----[500Ω]----┼---- [Master ESP32 MISO]
 *   [3rd Slave ESP32 MISO] ----[500Ω]----┘
 *
 */
const int spi_pins[] = {4, HSPI_CS, 16};
auto& spi_socket = S_Broadcast_SPI_2xESP_4MHz_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));


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
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(2000); // Important: Give time for serial to initialize

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

	// ================== INITIALIZE HSPI ==================
	// Initialize SPI with HSPI pins: MOSI=13, MISO=12, SCK=14
    spi_socket.begin(13, 12, 14);	// MOSI, MISO, SCK

    // Finally, sets the blue led as always HIGH signalling this way to be a SPI Master
    digitalWrite(LED_BUILTIN, HIGH);

	#ifdef SKETCH_DEBUG
        Serial.println("Setup completed - Ready for JSON communication!");
    #endif
}


void loop() {
	message_repeater.loop();	// Keep calling the Repeater
}
