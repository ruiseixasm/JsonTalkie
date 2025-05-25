# JsonTalkie - JSON-based Communication for Arduino

A lightweight library for Arduino communication and control using JSON messages over network sockets, with Python companion scripts for host computer interaction.

## Features

- Bi-directional JSON-based communication between Arduino and Python
- Simple command/response pattern with "Walkie-talkie" style interaction
- Device configuration with a Manifesto for self-describing capabilities
- Automatic command discovery and documentation
- Support for multiple devices on the same network

## Installation

### Arduino Library
1. **Using Arduino Library Manager**:
   - Open Arduino IDE
   - Go to `Sketch > Include Library > Manage Libraries`
   - Search for "JsonTalkie"
   - Click "Install"

2. **Manual Installation**:
   - Download the latest release from GitHub
   - Extract to your Arduino libraries folder
   - Restart Arduino IDE

### Dependencies
   - Requires [ArduinoJson](https://arduinojson.org/) (any recent version)

## Python Command Line
### JsonTalkiePy repository with command line as Talker
   - Talker in [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy)
   - Got the the page above for more details concerning its usage

### Typical usage
```bash
rui@acer:~/GitHub/JsonTalkie/Python$ python3.13 talk.py 
	[Talker-1b] running. Type 'exit' to exit or 'talk' to make them talk.
>>> talk
	[Talker-1b talk]     	A simple Talker!
	[Nano talk]          	I do a 500ms buzz!
>>> Nano sys
	[Nano sys]            	Arduino Uno/Nano (ATmega328P)
>>> Nano list
	[Nano run buzz]      	Triggers buzzing
	[Nano run on]        	Turns led On
	[Nano run off]       	Turns led Off
	[Nano get total_runs]	Gets the total number of runs
>>> Nano run buzz
	[Nano run buzz]      	ROGER
>>> help
	[talk]                  Prints all devices' 'name' and description.
	['device' list]         List the entire 'device' manifesto.
	['device' channel]      Shows the Device channel.
	['device' channel n]    Sets the Device channel.
	['device' run 'what']   Runs the named function.
	['device' set 'what']   Sets the named variable.
	['device' get 'what']   Gets the named variable value.
	[sys]                   Prints the platform of the Device.
	[port]                  Gets the Broadcast Socket port.
	[port n]                Sets the Broadcast Socket port.
	[exit]                  Exits the command line (Ctrl+D).
	[help]                  Shows the present help.
>>> exit
	Exiting...
```


## **Library functions**
### JsonTalkie
#### **`set_manifesto(Manifesto* manifesto)`**
This function sets the `Manifesto` to be used as the Device attributes and capabilities.
#### **`get_manifesto()`**
Returns the Manifesto set above.
#### **`plug_socket(BroadcastSocket* socket)`**
Sets which socket to be used as BroadcastSocket.
#### **`unplug_socket(BroadcastSocket* socket)`**
Removes any plugged socket.
#### **`talk(JsonObject message, bool as_reply = false)`**
Sends the message as a JsonObject to all existent devices or to a single device if `as_reply`.
#### **`listen()`**
This method processes received messages so it shall be called inside the `loop` function.
### BroadcastSocket
#### **`send(const char* data, size_t len, bool as_reply = false)`**
Sends the `char` data to all devices or to a single device if `as_reply` over the protocol used by the socket (Ex. UDP).
#### **`receive(char* buffer, size_t size)`**
Receives data from the socket, this method is called by the JsonTalkie `listen()` method, so, it doesn't need to be called.
#### **`set_port(uint16_t port)`**
This method sets a different `port` for the Socket being used.
#### **`get_port(uint16_t port)`**
This method returns the `port` number of the Socket being used.

## **Examples**
### **Usage of the UDP protocol in a Arduino Mega with an ENC28J60 module**
```Arduino
#include <JsonTalkie.hpp>
#include <sockets/BroadcastSocket_UIP.hpp>


JsonTalkie json_talkie;
auto& broadcast_socket = BroadcastSocket_UIP::instance();
EthernetUDP udp;

// Network Settings
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const unsigned int PORT = 5005;




// MANIFESTO DEFINITION

// Define the commands (stored in RAM)
JsonTalkie::Device device = {
    "Mega", "I do a 500ms buzz!"
};

bool buzz(JsonObject json_message);
bool led_on(JsonObject json_message);
bool led_off(JsonObject json_message);
JsonTalkie::Run runCommands[] = {
    {"buzz", "Triggers buzzing", buzz},
    {"on", "Turns led On", led_on},
    {"off", "Turns led Off", led_off}
};

bool set_duration(JsonObject json_message, long duration);
JsonTalkie::Set setCommands[] = {
    // {"duration", "Sets duration", set_duration}
};

long get_total_runs(JsonObject json_message);
long get_duration(JsonObject json_message);
JsonTalkie::Get getCommands[] = {
    {"total_runs", "Gets the total number of runs", get_total_runs}
    // {"duration", "Gets duration", get_duration}
};

bool process_response(JsonObject json_message);


// MANIFESTO DECLARATION

JsonTalkie::Manifesto manifesto(
    &device,
    runCommands, sizeof(runCommands)/sizeof(JsonTalkie::Run),
    setCommands, sizeof(setCommands)/sizeof(JsonTalkie::Set),
    getCommands, sizeof(getCommands)/sizeof(JsonTalkie::Get),
    process_response, nullptr
);

// END OF MANIFESTO



// Buzzer pin
#define buzzer_pin 3

void setup() {
    // Serial is a singleton class (can be began multiple times)
    Serial.begin(9600);
    while (!Serial);
    
    delay(2000);    // Just to give some time to Serial

    Serial.print("\n\nConnecting");
      
    // Start Ethernet connection
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP");
        while (1); // Halt if connection fails
    }

    // Start UDP
    if (udp.begin(PORT)) {
        Serial.print("\n\nUDP active on ");
        Serial.println(Ethernet.localIP());
    } else {
        Serial.println("UDP failed!");
        while (1); // Halt if connection fails
    }
    
    Serial.println("\n\nOpening the Socket...");
    
    // By default is already 5005
    broadcast_socket.set_port(5005);
    broadcast_socket.set_udp(&udp);

    json_talkie.set_manifesto(&manifesto);
    json_talkie.plug_socket(&broadcast_socket);


    Serial.println("Talker ready");

    #ifndef BROADCAST_SOCKET_SERIAL_HPP
    pinMode(buzzer_pin, OUTPUT);
    digitalWrite(buzzer_pin, HIGH);
    delay(10); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Sending JSON...");
}

void loop() {
    json_talkie.listen();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 39000) {

        JsonDocument message_doc;
        if (message_doc.overflowed()) {
            Serial.println("CRITICAL: Insufficient RAM");
        } else {
            JsonObject message = message_doc.to<JsonObject>();
            message["m"] = 0;   // talk
            json_talkie.talk(message);
        }
        
        lastSend = millis();
    }
}


long total_runs = 0;
long _duration = 5;  // Example variable

// Command implementations
bool buzz(JsonObject json_message) {
    #ifndef BROADCASTSOCKET_SERIAL
    digitalWrite(buzzer_pin, HIGH);
    delay(_duration); 
    digitalWrite(buzzer_pin, LOW);
    #endif
    total_runs++;
    return true;
}


bool is_led_on = false;  // keep track of state yourself, by default it's off

bool led_on(JsonObject json_message) {
    if (!is_led_on) {
        digitalWrite(LED_BUILTIN, HIGH);
        is_led_on = true;
        total_runs++;
    } else {
        json_message["r"] = "Already On!";
        json_talkie.talk(json_message);
        return false;
    }
    return true;
}

bool led_off(JsonObject json_message) {
    if (is_led_on) {
        digitalWrite(LED_BUILTIN, LOW);
        is_led_on = false;
        total_runs++;
    } else {
        json_message["r"] = "Already Off!";
        json_talkie.talk(json_message);
        return false;
    }
    return true;
}


bool set_duration(JsonObject json_message, long duration) {
    _duration = duration;
    return true;
}

long get_duration(JsonObject json_message) {
    return _duration;
}

long get_total_runs(JsonObject json_message) {
    return total_runs;
}


bool process_response(JsonObject json_message) {
    Serial.print(json_message["f"].as<String>());
    Serial.print(" - ");
    if (json_message.containsKey("r")) {
        Serial.println(json_message["r"].as<String>());
    } else if (json_message.containsKey("d")) {
        Serial.println(json_message["d"].as<String>());
    } else {
        Serial.println("Empty echo received!");
    }
    return false;
}
```


## **User defined BroadcastSocket**
### **How to include your own BroadcastSocket implementation**
You can always implement your own Socket besides the ones given by the examples. To do so, you need to implement
the `BroadcastSocket` abstract class (interface) with your own `BroadcastSocket_User` implementation.
Then you can include it this way.
```Arduino
#include <JsonTalkie.hpp>
#include "BroadcastSocket_User.hpp"
```
### **Using the JsonTalkie_Dummy for testing while developing**
In order to be easier the development of your own `BroadcastSocket` file, you can use the Dummy version of the JsonTalkie,
like this:
```Arduino
#include <dummies/JsonTalkie_Dummy.hpp>
#include <ArduinoJson.h>    // Includes ArduinoJson Library NOT included in the Dummy file above
#include "BroadcastSocket_User.hpp"
```
The `JsonTalkie_Dummy` periodically sends typical `JsonTalkie` messages as if it was the real deal.
### **Example of a user defined BroadcastSocket file**
Here is an example of such implementation that must be side-by-side with your `.ino` file.
```Arduino
#ifndef BROADCAST_SOCKET_USER_HPP
#define BROADCAST_SOCKET_USER_HPP

#include <BroadcastSocket.hpp>
#include <ArduinoJson.h>


// Readjust if absolutely necessary
#define BROADCAST_USER_DEBUG


class BroadcastSocket_User : public BroadcastSocket {
private:
    static unsigned long _lastTime;

    // BroadcastSocket_User() {
    //     Serial.println(F("[WARNING] Using Dummy Socket Implementation"));
    //     Serial.println(F("[WARNING] No Ethernet library or custom socket defined"));
    //     Serial.println(F("[WARNING] All network operations will be no-ops"));
    // }

    // Helper function to safely create char* from buffer
    static char* decode(const uint8_t* data, const size_t length, char* talk) {
        memcpy(talk, data, length);
        talk[length] = '\0';
        return talk;
    }

    bool valid_checksum(JsonObject message, char* buffer, size_t size) {
        // Use a static buffer size, large enough for your JSON
        uint16_t message_checksum = 0;
        if (message.containsKey("c")) {
            message_checksum = message["c"].as<uint16_t>();
        }
        message["c"] = 0;
        size_t len = serializeJson(message, buffer, size);

        if (len == 0) {
            #ifdef BROADCAST_USER_DEBUG
            Serial.println("ERROR: Checksum serialization failed!");
            #endif
            return false;
        }

        #ifdef BROADCAST_USER_DEBUG
        // DEBUG: Print buffer contents
        Serial.println("Buffer contents:");
        for (size_t i = 0; i < len; i++) {
            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        #endif

        // 16-bit word and XORing
        uint16_t checksum = 0;
        for (size_t i = 0; i < len; i += 2) {
            uint16_t chunk = buffer[i] << 8;
            if (i + 1 < len) {
                chunk |= buffer[i + 1];
            }
            checksum ^= chunk;
        }

        #ifdef BROADCAST_USER_DEBUG
        Serial.print("Message checksum: ");
        Serial.println(checksum);  // optional: just to add a newline after the JSON
        #endif

        message["c"] = checksum;
        return message_checksum == checksum;
    }

public:
    // Singleton accessor
    static BroadcastSocket_User& instance() {
        static BroadcastSocket_User instance;
        return instance;
    }


    bool send(const char* data, size_t len, bool as_reply = false) override {
        #ifdef BROADCAST_USER_DEBUG
        Serial.print(F("DUMMY SENT: "));
        char talk[len + 1];
        Serial.println(decode(data, len, talk));
        #endif
        return true;
    }

    
    size_t receive(char* buffer, size_t size) override {
        if (millis() - _lastTime > 1000) {
            _lastTime = millis();
            if (random(1000) < 100) { // 10% chance
                // 2. Message Selection
                // ALWAYS VALIDATE THE MESSAGES FOR BAD FORMATING !!
                const char* PROGMEM messages[] = {
                    R"({"m":0,"f":"Dummy","i":3003412860})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","n":"buzz","i":3003412861})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","n":"on","i":3003412862})",
                    R"({"m":2,"f":"Dummy","t":"Buzzer","n":"off","i":3003412863})",
                    R"({"m":6,"f":"Dummy","t":"*","r":"Dummy echo","i":3003412864})",
                    R"({"m":6,"f":"Dummy","t":"*","r":"Broadcasted echo","i":3003412865})",
                    R"({"m":6,"f":"Dummy","t":"*","r":"Direct echo","i":3003412866})"
                };
                const size_t num_messages = sizeof(messages)/sizeof(char*);
                
                // 3. Safer Random Selection
                const char* message_char = messages[random(num_messages)];
                size_t message_size = strlen(message_char);
                
                // 5. JSON Handling with Memory Checks

                JsonDocument message_doc;
                if (message_doc.overflowed()) {
                    Serial.println(F("Failed to allocate JSON message_doc"));
                    return;
                }
                
                // Message needs to be '\0' terminated and thus buffer is used instead
                // it's possible to serialize from a JsonObject but it isn't to deserialize into a JsonObject!
                DeserializationError error = deserializeJson(message_doc, message_char, message_size);
                if (error) {
                    Serial.println(F("Failed to deserialize message"));
                    return;
                }
                JsonObject message = message_doc.as<JsonObject>();
                valid_checksum(message, buffer, size);

                size_t message_len = serializeJson(message, buffer, size);
                if (message_len == 0 || message_len >= size) {
                    Serial.println(F("Serialization failed/buffer overflow"));
                    return;
                }

                #ifdef BROADCAST_USER_DEBUG
                Serial.print("DUMMY RECEIVED: ");
                serializeJson(message, Serial);
                Serial.println();  // optional: just to add a newline after the JSON
                #endif

                return message_len;
            }
        }
        return 0;
    }
};

static unsigned long BroadcastSocket_User::_lastTime = 0;

#endif // BROADCAST_SOCKET_USER_HPP
```
