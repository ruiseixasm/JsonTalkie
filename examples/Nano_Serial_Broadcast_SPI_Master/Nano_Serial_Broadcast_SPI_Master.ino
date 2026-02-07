/**
 * @file    Nano_Serial_Broadcast_SPI_Master.ino
 * @author  Rui Seixas Monteiro
 * @brief   An Ethernet ENC28J60 shield connected to an Arduino Mega.
 *
 * This sketch demonstrates how you can implement the EthernetENC adapted library
 * able to work in Broadcast mode
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * 
 * Hardware:
 * - One Arduino Mega board and an Ethernet ENC28J60 shield
 * 
 * NOTE:
 * - In the Arduino Mega, you should set the pin 53 as OUTPUT, and the pin 10 as the CS pin.
 *
 * Sockets:
 * - S_Broadcast_SPI_2xArduino_Master
 * - S_SocketSerial
 * 
 * Manifestos:
 * - M_BuzzerManifesto
 * 
 * Created: 2026-02-07
 */

#include <JsonTalkie.hpp>
#include "S_Broadcast_SPI_2xArduino_Master.hpp"
#include "S_SocketSerial.hpp"
#include "M_BuzzerManifesto.hpp"


const char talker_name[] = "broadcast";
const char talker_desc[] = "I'm a broadcast talker";
M_BuzzerManifesto buzzer_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &buzzer_manifesto);

// Singleton requires the & (to get a reference variable)
auto& serial_socket = S_SocketSerial::instance();
const int spi_pins[] = {SS};	// In this case it's a single SS pin being used because are just two boards paired
auto& spi_socket = S_Broadcast_SPI_2xArduino_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &serial_socket };
JsonTalker* downlinked_talkers[] = { &talker };
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
    delay(250); // Important: Give time for serial to initialize
    Serial.println("\n\n=== Arduino with SERIAL ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    message_repeater.loop();
}

