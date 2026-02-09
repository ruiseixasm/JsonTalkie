/**
 * @file    NanoBuzzer_Broadcast_SPI_Slave.ino
 * @author  Rui Seixas Monteiro
 * @brief   This is a SPI Slave that can be connected to any SPI Master with an SPI equivalent Socket.
 *
 * This sketch has a buzzer controlled via pin 2 able to receive messages via SPI to trigger it.
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
 *   with multiple Arduino boards as SPI Slaves, make sure you connect a resistor of **around 500 Ohms** to
 *   each SPI Slave MISO pin, in the case of the Arduino Uno and Nano is the pin 12!
 * 
 *   [1st Slave Arduino MISO] ----[500Ω]----┐
 *   [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master Arduino MISO]
 *   [3rd Slave Arduino MISO] ----[500Ω]----┘
 *
 * Sockets:
 * - S_Broadcast_SPI_Arduino_Slave
 * 
 * Manifestos:
 * - M_BuzzerManifesto
 * 
 * Created: 2026-02-09
 */

#include <JsonTalkie.hpp>
#include "S_Broadcast_SPI_Arduino_Slave.h"
#include "M_BuzzerManifesto.hpp"


const char talker_name[] = "buzzer";
const char talker_desc[] = "I'm a buzzer that buzzes";
M_BuzzerManifesto talker_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &talker_manifesto);

// Singleton requires the & (to get a reference variable)
auto& spi_socket = S_Broadcast_SPI_Arduino_Slave::instance();

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &spi_socket };
JsonTalker* downlinked_talkers[] = { &talker };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);



void setup() {
    // Then start Serial
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n\n=== Arduino with SPI STARTING ==="));

    Serial.println(F("Step 1: Starting SPI..."));
	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too
    Serial.println(F("SPI started successfully"));
    delay(5000);	// Waits 5 seconds for all other SPIs to start (it's a slave, so, it depends on a master)

    Serial.println(F("Setup completed - Ready for JSON communication!"));
}



void loop() {
    message_repeater.loop();
}

