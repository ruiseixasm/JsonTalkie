# JsonTalkie - Broadcast Sockets

Here you can find many Sockets that are examples of `BroadcastSocket` interface implementation.

To include any of these Sockets, just add the following include line accordingly:
```cpp
#include "S_BroadcastESP_WiFi.hpp"
```

## Implementation
You can always implement your own socket, by extending the `BroadcastSocket` class. To do so, you must override the following methods:
```cpp
	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
	const char* class_description() const override { return "YourSocketName"; }
	void _receive() override {}
	bool _send(const JsonMessage& json_message) override {}
```
The methods above should follow these basic rules:
- In the `_receive` method you must create a `JsonMessage` and write on its buffer or deserialize on it the data received, if you write directly on the created `JsonMessage` buffer
you shall not forget to set its length with the method `_set_length`. After that, it should be called the method `_startTransmission` to process the received data. See example bellow
for details;
- In the `_send` method you must read from the `json_message` buffer with the help of the methods `_read_buffer` and `get_length` or just serialize it into a buffer.
## Example
Here is an example of such implementation for the Serial protocol:
```cpp
#ifndef SOCKET_SERIAL_HPP
#define SOCKET_SERIAL_HPP

#include <BroadcastSocket.h>


class S_SocketSerial : public BroadcastSocket {
public:

	// The Socket class description shouldn't be greater than 35 chars
	// {"m":7,"f":"","s":3,"b":1,"t":"","i":58485,"0":1,"1":"","2":11,"c":11266} <-- 128 - (73 + 2*10) = 35
    const char* class_description() const override { return "SocketSerial"; }

	#ifdef SOCKET_SERIAL_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif


protected:

    // Singleton accessor
    S_SocketSerial() : BroadcastSocket() {}

	JsonMessage _json_message;
	bool _reading_serial = false;


    void _receive() override {
    
		while (Serial.available()) {
			char c = Serial.read();

			char* message_buffer = _json_message._write_buffer();
			if (_reading_serial) {

				size_t message_length = _json_message.get_length();
				if (message_length < TALKIE_BUFFER_SIZE) {
					if (c == '}' && message_length && message_buffer[message_length - 1] != '\\') {

						_reading_serial = false;

						if (_json_message._append('}')) {
							_startTransmission(_json_message);
						}
						return;
					} else if (!_json_message._append(c)) {
						return;
					}
				} else {
					_reading_serial = false;
					_json_message._set_length(0);	// Reset to start writing
				}
			} else if (c == '{') {
				
				_json_message._set_length(0);
				_reading_serial = true;

				_json_message._append('{');
			}
		}
    }


    bool _send(const JsonMessage& json_message) override {

		const char* message_buffer = json_message._read_buffer();
		size_t message_length = json_message.get_length();
		return Serial.write(message_buffer, message_length) == message_length;
    }


public:
    // Move ONLY the singleton instance method to subclass
    static S_SocketSerial& instance() {

        static S_SocketSerial instance;
        return instance;
    }

};

#endif // SOCKET_SERIAL_HPP
```

You can find many other examples of implementation in this folder.

## Ethernet
The Ethernet Broadcast Socket uses the UDP protocol and the port `5005` by default, however, you can always set a different UDP port
if you wish to separate multiple Talkers from each other by aggregating them by UDP port.

You can set a specific UDP port like so:
```cpp
	ethernet_socket.set_port(5001);	// Able to set a specific udp port
```

Then you can just start the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program like so:
```sh
python talk.py --socket UDP --port 5001
```
Otherwise, to use the default `5005` port, just type:
```sh
python talk.py
```

### S_BroadcastSocket_EtherCard
For this particular Socket a different port than the default `5005`, has to be set in the constructor, like so:
```cpp
auto& ethernet_socket = S_BroadcastSocket_EtherCard::instance(5001);
```
#### Description
Lightweight socket intended to be used with low memory boards like the Uno and the Nano, for the ethernet module `ENC28J60`.
This library has the limitation of not being able to send *unicast* messages, all its responses are in *broadcast* mode, so, one
way to avoid it is to mute the Talker and in that way the echoes to the `call` commands on it don't overload the network.
```
>>> list nano
    [call nano 0|buzz]         Buzz for a while
    [call nano 1|ms]           Gets and sets the buzzing duration
>>> call nano 0
    [call nano 0]              roger
>>> system nano mute 1
    [system nano mute]         1
>>> call nano 0
>>>
```
Not being able to work in unicast means that the replies will have high latency or not even reach the source if sent over WiFi.
```
>>> talk uno
>>> talk uno
	[talk uno]           	   Arduino Uno
>>> ping uno
>>> ping uno
	[ping uno]           	   106
>>>
```
Looking above, besides a long ping, 106 milliseconds, some replies weren't even received, so, this library shouldn't be
used over WiFi if replies are critical. Must be noted however, that this huge latency is mainly due to the broadcasted reply,
meaning that the received unicast (by name) message has the usual small latency regardless the receiving Ethernet socket type.
#### Dependencies
This Socket depends on the [library EtherCard](https://github.com/njh/EtherCard) that you can instal directly from the Arduino IDE.
### S_EthernetENC_Broadcast
#### Description
This is the best library to be used with the module `ENC28J60`, it requires more memory so it shall be used with an
Arduino Mega or any other with similar amount of memory or more.

One of it's problems is that relies in a change done to the original library, as it's one that doesn't respond to broadcasted UDP packets
out of the box. To do so, you can pick up the already changed one [here](https://github.com/ruiseixasm/S_EthernetENC_Broadcast),
or you can tweak the original yourself, by changing these lines in the file `Enc28J60Network.cpp`.

Comment the existing lines and add the new one as bellow:
```cpp
    // Pattern matching disabled – not needed for broadcast and multicast
	// writeReg(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
    // writeRegPair(EPMM0, 0x303f);
    // writeRegPair(EPMCSL, 0xf7f9);
    writeReg(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN | ERXFCON_MCEN);
```
After the change above, the Socket using this library will also receive broadcasted UDP messages.
For more details check the data sheet of the chip [ENC28J60](https://ww1.microchip.com/downloads/en/devicedoc/39662a.pdf).
Contrary to the socket implementation `S_BroadcastSocket_EtherCard` described above, this socket does *unicast*, so, it can be used over WiFi without the extra latency referred above, 4 instead of 106 milliseconds.
Even though, this extra latency only concerns the `echo` message.
```
>>> talk spy
	[talk spy]           	   I'm a Spy and I spy the talkers' pings
>>> ping spy
	[ping spy]           	   4
>>>
```
#### Dependencies
This library is the adaptation of the EthernetENC library that allows the needed broadcast via UDP. You need to download the zip from
the [EthernetENC_Broadcast](https://github.com/ruiseixasm/EthernetENC_Broadcast) repository as zip and then unzip it in the *libraries* Arduino folder.
### S_BroadcastSocket_Ethernet
#### Description
This socket is intended to be used with the original [Arduino Ethernet board](https://docs.arduino.cc/retired/shields/arduino-ethernet-shield-without-poe-module/),
or other that has the chip `W5500` or `W5100`. Take note that depending on the board, the pins may vary,
so read the following note first.
```
Arduino communicates with both the W5100 and SD card using the SPI bus (through the ICSP header).
This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. On both boards,
pin 10 is used to select the W5100 and pin 4 for the SD card. These pins cannot be used for general I/O.
On the Mega, the hardware SS pin, 53, is not used to select either the W5100 or the SD card,
but it must be kept as an output or the SPI interface won't work.
```
Contrary to the socket implementation `S_BroadcastSocket_EtherCard` described above, this socket does *unicast*, so,
it can be used via Wi-Fi too without the latency referred above, 6 instead of 106 milliseconds.
```
>>> talk mega
	[talk mega]          	   I'm a Mega talker
>>> ping mega
	[ping mega]          	   6
>>>
```
#### Dependencies
This Socket depends on the [Ethernet library](https://github.com/arduino-libraries/Ethernet), so, you need to install it with the Arduino IDE.

## WiFi
The WiFi Broadcast Socket uses the UDP protocol and the port `5005` by default, however, you can always set a different UDP port
if you wish to separate multiple Talkers from each other aggregating them by UDP port.

You can set a specific UDP port like so:
```cpp
	wifi_socket.set_port(5001);	// Able to set a specific udp port
```

Then you can just start the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program like so:
```sh
python talk.py --socket UDP --port 5001
```
Otherwise just type:
```sh
python talk.py
```

### S_BroadcastESP_WiFi
#### Description
This socket is intended to be used with the boards ESP8266 or ESP32 that come with WiFi out of the box.

Contrary to the socket implementation `S_BroadcastSocket_EtherCard` described above, this socket does *unicast*, so,
it can be used over WiFi too without the respective broadcast extra latency, 3 instead of 106 milliseconds.
```
>>> talk blue
	[talk blue]          	   I turn led Blue on and off
>>> ping blue
	[ping blue]          	   3
>>>
```
One thing to take into consideration though, is that the fist ping can be sent in Broadcast mode because the first command normally has no info about the Talker IP, so the first command is sent in Broadcast and
used to get the Talker address, afterwards the messages are sent in unicast mode to the same Talker's name.
So, in the example above, the `talk blue` command resulted in the association of the IP address with the Talker's name enabling the fast unicast `ping blue` command that follows, only 3 milliseconds.
#### Dependencies
By installing the ESP8266 or ESP32 boards, you already have the WiFi library available.

## SPI
All these Socket implementations use the out of the box installed Arduino `SPI.h` or the ESP32
`spi_master.h`/`spi_slave.h` library, so, no need to extra installs.

### Broadcast
These Sockets are intended to work in Broadcast mode, meaning, while the SPI Master is sending a message on its MOSI pin, all SPI Slaves are **simultaneously**
receiving that same message, this happens because the SPI Master switches its `SS` pin to low on *all* SPI Slave devices at once. This will result in *all* SPI Slaves responding
at the same time on their MISO pin, so, if those pins are directly connected, this will damage the SPI Slaves' MISO pin, and to avoid it, you have to add a resistor of **around 500 Ohms** to each SPI Slave MISO pin, like so:
```
	[1st Slave MISO] ----[500Ω]----┐
	[2nd Slave MISO] ----[500Ω]----┼---- [Master MISO]
	[3rd Slave MISO] ----[500Ω]----┘
```
The communication is thus done in *half-duplex*, where the SPI Master doesn't read any message while sending and *vice versa*. This reflects the nature of the *JsonTalkie* protocol in
prioritizing the Broadcast type of messages over the Reply ones.
#### S_Broadcast_SPI_2xArduino_Master
This Socket is intended to be used in an Arduino board that will work as a SPI Master of multiple **Arduino boards** as SPI Slaves.

#### S_Broadcast_SPI_Arduino_Slave
This Socket is intended to be used in an **Arduino board** as a SPI Slave.

#### S_Broadcast_SPI_ESP_Arduino_Master
This Socket is intended to be used in an ESP32 board that will work as a SPI Master of multiple **Arduino boards** as SPI Slaves.

#### S_Broadcast_SPI_2xESP_4MHz_Master
This Socket is intended to be used in an ESP32 board that will work as a SPI Master of multiple **ESP32 boards** as SPI Slaves.

#### S_Broadcast_SPI_2xESP_4MHz_Slave
This Socket is intended to be used in an **ESP32 board** as a SPI Slave.

### Basic
The Basic version of the SPI Socket is one that doesn't work in Broadcast mode, so, in a more traditional way, it sends messages to each SPI Slave one by one separately, meaning that,
when you have multiple devices as SPI Slaves, the latency starts to add up, this happens because a single message has to be sent to each SPI Slave Device individually.
On the other hand, no protection resistor on the MISO pins is needed, so, use these these *Basic* sockets if you absolutely can't add the referred **500 Ohms resistor**.

#### S_Basic_SPI_2xArduino_Master_Multiple
This Socket allows the communication centered in a single Arduino SPI master board to many **Arduino boards** as SPI slave.

#### S_Basic_SPI_2xArduino_Master
This Socket allows the communication centered in a single Arduino SPI master board to another single **Arduino board** as SPI slave.

#### S_Basic_SPI_Arduino_Slave
This Socket is targeted to **Arduino boards** intended to be used as SPI Slaves.

#### S_Basic_SPI_ESP_Arduino_Master
This Socket allows the communication centered in a single ESP32 SPI master board to many **Arduino boards** as SPI slave.


## Serial
You can just start the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program like so:
```sh
python talk.py --socket SERIAL --port COM4
```
### S_SocketSerial
#### Description
This is the simplest Socket of all, and it's not a Broadcast Socket at all, given that the Serial protocol is one-to-one, so, it's main purpose is just that, for two boards to communicate with each other, ideal for local communication in the same platform where speed is not required.

It may be used for testing too, by allowing a direct connection with a computer. Just make sure you disable any Serial print as those
may interfere with the normal flow of Json messages if formatted in the same way as a json string.
#### Dependencies
This uses the Serial communication, so, no need to install any extra library.

