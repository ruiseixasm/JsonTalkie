/**
 * @file    Nano_Serial_Broadcast_SPI_Master.ino
 * @author  Rui Seixas Monteiro
 * @brief   Two SPI paired Nano boards with one being the Master and the other the Slave.
 *
 * This sketch demonstrates how you can connect two Arduino boards via SPI where the Master
 * can be accessed via Serial and is able to control the other Arduino Slave board as SPI Master.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Hardware:
 * - Two Arduino boards or more if you pretend to work in Broadcast mode (check CAUTION bellow)
 * 
 * CAUTION:
 * - With this sketch you can have more than one single board as SPI Slave, this happens because the
 *   `S_Broadcast_SPI_2xArduino_Master` socket is a broadcast socket. However, if you choose to work
 *   with multiple Arduino boards as SPI Slave, make sure you connect a resistor of **around 500 Ohms** to
 *   each SPI Slave MISO pin, in the case of the Arduino Uno and Nano is the pin 12!
 * 
 *   [1st Slave Arduino MISO] ----[500Ω]----┐
 *   [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master Arduino MISO]
 *   [3rd Slave Arduino MISO] ----[500Ω]----┘
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
/**
 * CAUTION:
 * - For more than one single board as SPI Slave, make sure you connect a resistor of **around 500 Ohms** to
 *   each SPI Slave MISO pin, in the case of the Arduino Uno and Nano is the pin 12!
 * 
 *   [1st Slave Arduino MISO] ----[500Ω]----┐
 *   [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master Arduino MISO]
 *   [3rd Slave Arduino MISO] ----[500Ω]----┘
 *
 */
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

