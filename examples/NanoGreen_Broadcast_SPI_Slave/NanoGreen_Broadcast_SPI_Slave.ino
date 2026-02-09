/**
 * @file    NanoGreen_Broadcast_SPI_Slave.ino
 * @author  Rui Seixas Monteiro
 * @brief   This is a SPI Slave that can be connected to any SPI Master with an SPI equivalent Socket.
 *
 * This sketch has a Yellow led controlled via pin 19 able to receive messages via SPI to turn it `on` and `off`.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - Single Arduino board able to work as SPI Slave (check CAUTION bellow)
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
 * - M_GreenManifesto
 * - M_LedManifesto
 * 
 * Created: 2026-02-09
 */

// GREEN_LED is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef GREEN_LED
  #define GREEN_LED 2  // Fallback definition if not already defined
#endif

#ifndef YELLOW_LED_PIN
#define YELLOW_LED_PIN 19   // A5
#endif


#include <JsonTalkie.hpp>
#include "S_Broadcast_SPI_Arduino_Slave.h"
#include "M_GreenManifesto.hpp"
#include "M_LedManifesto.hpp"


const char green_name[] = "green";
const char green_desc[] = "I'm a green talker";
M_GreenManifesto green_manifesto;
JsonTalker green = JsonTalker(green_name, green_desc, &green_manifesto);

const char yellow_name[] = "yellow";
const char yellow_desc[] = "I'm a yellow talker";
M_LedManifesto yellow_manifesto(YELLOW_LED_PIN);
JsonTalker yellow = JsonTalker(yellow_name, yellow_desc, &yellow_manifesto);

// Singleton requires the & (to get a reference variable)
auto& spi_socket = S_Broadcast_SPI_Arduino_Slave::instance();

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &spi_socket };
JsonTalker* downlinked_talkers[] = { &green, &yellow };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(GREEN_LED, OUTPUT);
    digitalWrite(GREEN_LED, LOW); // Start with LED off
	pinMode(YELLOW_LED_PIN, OUTPUT);
	digitalWrite(YELLOW_LED_PIN, LOW);

    // Then start Serial
    Serial.begin(115200);
    delay(100); // Important: Give time for serial to initialize
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    Serial.println(F("\n\n=== Arduino with SPI STARTING ==="));

    // Add a small LED blink to confirm code is running
    digitalWrite(GREEN_LED, HIGH);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
    delay(100);
    digitalWrite(GREEN_LED, HIGH);
    delay(100);
    digitalWrite(GREEN_LED, LOW);
    
    Serial.println(F("Pins initialized successfully"));



    // STEP 1: Initialize SPI only
    // Defines the CS pin by Talker name here
	
    Serial.println(F("Step 1: Starting SPI..."));
	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too
    Serial.println(F("SPI started successfully"));

    // Final startup indication
    digitalWrite(GREEN_LED, HIGH);
    delay(500);
    digitalWrite(GREEN_LED, LOW);
    delay(5000);	// Waits 5 seconds for all other SPIs to start (it's a slave, so, it depends on a master)

    Serial.println(F("Setup completed - Ready for JSON communication!"));
}


void loop() {
    message_repeater.loop();
}

