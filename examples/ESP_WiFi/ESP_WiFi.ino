/**
 * @file    TalkieESP_WiFi.ino
 * @author  Rui Seixas Monteiro
 * @brief   An WiFi Socket where many Manifestos are used.
 *
 * This sketch demonstrates how you can remotely turn on and off the board led.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/examples
 * 
 * Hardware:
 * - An ESP8266, ESP8285 or ESP32 board with a built-in LED (LED_BUILTIN)
 *
 * Sockets:
 * - S_BroadcastESP_WiFi
 * 
 * Manifestos:
 * - M_BlueManifesto
 * - M_Esp66Manifesto
 * - M_MessageTester
 * - M_Spy
 * 
 * Created: 2026-01-15
 */


#include <JsonTalkie.hpp>
#include "S_BroadcastESP_WiFi.hpp"
#include "M_BlueManifesto.hpp"
#include "M_Esp66Manifesto.hpp"
#include "M_MessageTester.hpp"
#include "M_Spy.hpp"


const char ssid[] = "wifiName";
const char password[] = "wifiPassword";


#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif


// TALKERS 

// The Talker pair name and description shouldn't be greater than 54 chars
// {"m":7,"b":1,"i":20972,"f":"","t":"","0":"","c":13173} <-- 128 - (54 + 2*10) = 54


// M_Spy Talker
const char t_spy_name[] = "spy2";
const char t_spy_desc[] = "I'm a Spy and I spy the talkers' pings";
M_Spy spy_manifesto;
JsonTalker t_spy = JsonTalker(t_spy_name, t_spy_desc, &spy_manifesto);

// Talker (blue led)
const char l_blue_name[] = "blue2";
const char l_blue_desc[] = "I turn led Blue on and off";
M_BlueManifesto blue_manifesto(2);
JsonTalker l_blue = JsonTalker(l_blue_name, l_blue_desc, &blue_manifesto);

// Talker (JsonMessage tester)
const char t_tester_name[] = "test2";
const char t_tester_desc[] = "I test the JsonMessage class";
M_MessageTester message_tester;
JsonTalker t_tester = JsonTalker(t_tester_name, t_tester_desc, &message_tester);

// Talker (JsonMessage tester)
const char t_esp_name[] = "esp";
const char t_esp_desc[] = "I call on and off on the buzzer";
M_Esp66Manifesto caller_esp;
JsonTalker t_esp = JsonTalker(t_esp_name, t_esp_desc, &caller_esp);


// SOCKETS

// Singleton requires the & (to get a reference variable)
auto& ethernet_socket = S_BroadcastESP_WiFi::instance();


// SETTING THE REPEATER
BroadcastSocket* uplinked_sockets[] = { &ethernet_socket };
JsonTalker* downlinked_talkers[] = { &t_spy, &t_tester, &l_blue, &t_esp };
const MessageRepeater message_repeater(
		uplinked_sockets, sizeof(uplinked_sockets)/sizeof(BroadcastSocket*),
		downlinked_talkers, sizeof(downlinked_talkers)/sizeof(JsonTalker*)
	);


WiFiUDP udp;

// Network settings
#define PORT 5005   // UDP port


void setup() {
    // Initialize pins FIRST before anything else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Start with LED off
    
    // Then start Serial
    Serial.begin(115200);
    delay(1000); // Important: Give time for serial to initialize
    Serial.println("\n\n=== ESP with WiFi STARTING ===");

    // Add a small LED blink to confirm code is running
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    
    // STEP 1: Initialize WiFi
    Serial.println("Step 1: Initializing WiFi...");
    WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(250);
		Serial.print(".");
	}
	Serial.println("\n\tWiFi connected!");

    // STEP 2: Check connection status
    Serial.println("Step 2: Checking WiFi status...");
    Serial.print("\tLocal IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("\tSubnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("\tGateway IP: ");
    Serial.println(WiFi.gatewayIP());

    // STEP 3: Initialize UDP and broadcast socket
    Serial.println("Step 3: Initializing UDP...");
    if (udp.begin(PORT)) {
        Serial.println("\tUDP started successfully on port " + String(PORT));
    } else {
        Serial.println("\tFailed to start UDP!");
    }

    // STEP 5: Setting up broadcast sockets
    Serial.println("Step 4: Setting up broadcast sockets...");
    ethernet_socket.set_port(PORT);
    ethernet_socket.set_udp(&udp);

    // Final startup indication
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

	JsonMessage turn_off;
	turn_off.set_message_value(MessageValue::TALKIE_MSG_CALL);
	turn_off.set_to_name("blue2");	// The blue Talker is the one that controls the blue light
	turn_off.set_action_name("on");	// Because 'on' is 'off' on the ESP8266

	// ESP8266 has the HIGH and LOW inverted, so, the LED has to be set as HIG to be turned off
	#if defined(ESP8266)

	Serial.println("\tSending 'turn_off' message");
	message_repeater.downlinkMessage(turn_off);

	#endif
	
    Serial.println("Setup completed - Ready for JSON communication!");
}


void loop() {
	message_repeater.loop();
}

