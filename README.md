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
#### **`listen(bool receive = true)`**
This method processes received messages so it shall be called inside the `loop` function.
In case you create more than one JsonTalkie object, only one listen should call receive, thus,
all but one shall be `listen(false)` so that all can process the received data from the Socket.
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
#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include <Ethernet.h>
#include <EthernetUdp.h>


// #define BROADCAST_USER_DEBUG
#define ENABLE_DIRECT_ADDRESSING


class BroadcastSocket_User : public BroadcastSocket {
private:
    static IPAddress _source_ip;
    static EthernetUDP* _udp;

public:
    // Singleton accessor
    static BroadcastSocket_User& instance() {
        static BroadcastSocket_User instance;
        return instance;
    }

    
    bool send(const char* data, size_t size, bool as_reply = false) override {
        if (_udp == nullptr) return false;

        IPAddress broadcastIP(255, 255, 255, 255);
        
        #ifdef ENABLE_DIRECT_ADDRESSING
        if (!_udp->beginPacket(as_reply ? _source_ip : broadcastIP, _port)) {
            #ifdef BROADCAST_USER_DEBUG
            Serial.println(F("Failed to begin packet"));
            #endif
            return false;
        }
        #else
        if (!_udp->beginPacket(broadcastIP, _port)) {
            #ifdef BROADCAST_USER_DEBUG
            Serial.println(F("Failed to begin packet"));
            #endif
            return false;
        }
        #endif

        size_t bytesSent = _udp->write(reinterpret_cast<const uint8_t*>(data), size);

        if (!_udp->endPacket()) {
            #ifdef BROADCAST_USER_DEBUG
            Serial.println(F("Failed to end packet"));
            #endif
            return false;
        }

        #ifdef BROADCAST_USER_DEBUG
        Serial.print(F("S: "));
        Serial.write(data, size);
        Serial.println();
        #endif

        return true;
    }


    size_t receive(char* buffer, size_t size) override {
        if (_udp == nullptr) return 0;
        // Receive packets
        int packetSize = _udp->parsePacket();
        if (packetSize > 0) {
            int length = _udp->read(buffer, min(static_cast<size_t>(packetSize), size));
            if (length <= 0) return 0;  // Your requested check - handles all error cases
            
            #ifdef BROADCAST_USER_DEBUG
            Serial.print(packetSize);
            Serial.print(F("B from "));
            Serial.print(_udp->remoteIP());
            Serial.print(F(":"));
            Serial.print(_udp->remotePort());
            Serial.print(F(" -> "));
            Serial.println(buffer);
            #endif
            
            _source_ip = _udp->remoteIP();
            return jsonStrip(buffer, static_cast<size_t>(length));
        }
        return 0;   // nothing received
    }

    void set_udp(EthernetUDP* udp) { _udp = udp; }
};

IPAddress BroadcastSocket_User::_source_ip(0, 0, 0, 0);
EthernetUDP* BroadcastSocket_User::_udp = nullptr;


#endif // BROADCAST_SOCKET_USER_HPP
```
