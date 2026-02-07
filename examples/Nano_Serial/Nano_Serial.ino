/**
 * @file    Nano_Serial.ino
 * @author  Rui Seixas Monteiro
 * @brief   A single Arduino board that uses it's Serial port to communicate.
 *
 * This sketch demonstrates the usage of a single Arduino board via Serial, the simplest way of usage
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * 
 * Hardware:
 * - One Arduino or ESP board
 *
 * NOTE:
 * - The serial communication baud is always configured as `115200`
 * 
 * Sockets:
 * - S_SocketSerial
 * 
 * Manifestos:
 * - M_SerialManifesto
 * 
 * Created: 2026-02-07
 */

#include <JsonTalkie.hpp>
#include "S_SocketSerial.hpp"
#include "M_SerialManifesto.hpp"


const char talker_name[] = "serial";
const char talker_desc[] = "I'm a serial talker";
M_SerialManifesto serial_manifesto;
JsonTalker talker = JsonTalker(talker_name, talker_desc, &serial_manifesto);

// Singleton requires the & (to get a reference variable)
auto& serial_socket = S_SocketSerial::instance();

// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &serial_socket };
JsonTalker* downlinked_talkers[] = { &talker };
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
    delay(250); // Important: Give time for serial to initialize
    Serial.println("\n\n=== Arduino with SERIAL ===");

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
    message_repeater.loop();
}

