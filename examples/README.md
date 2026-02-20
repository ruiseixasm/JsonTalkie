# JsonTalkie - Talker Examples

Here is the description and purpose of the Examples above, where given [Sockets](https://github.com/ruiseixasm/JsonTalkie/tree/main/spckets)
and [Manifestos](https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos) are used.

The command line shown bellow is from the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program.

## ESP_WiFi
This is an Example where the WiFi is used as the Socket interface to the Talker. By being an WiFi Socket it has limitations concerning the Broadcast usage, because WiFi protocol itself has restrictions on Broadcasted packages (IP 255.255.255.255), this means that Broadcasted commands will have high delays or can even be simply
dropped. In order to send non Broadcasted commands make sure you add the name of the Talker in each command, like so:
```
>>> talk esp
    [talk esp]                 I call on and off on the buzzer
>>> list esp
    [call esp 0|active]        Gets or sets the active status
    [call esp 1|minutes]       Gets or sets the actual minutes
    [call esp 2|state]         The actual state of the led
    [call esp 3|enable]        Enables 1sec cyclic transmission
    [call esp 4|disable]       Disables 1sec cyclic transmission
>>> ping esp
    [ping esp]                 3
>>> call esp state
    [call esp state]           roger           1
>>> system esp board
    [system esp board]         ESP8266 (ID 1253476)
>>>
```
As you can see above, by adding the talker name, no hig delays or drops occurred, on the other hand, by using a broadcast message, without a named Talker like in the case of a channel, those delays will start to happen, due to the WiFi Broadcast restrictions referred above.

In any case, the ping is clearly higher, given the low priority of broadcasted message over WiFi.
```
>>> channel esp
    [channel esp]              255
>>> channel esp 2
    [channel esp]              2
>>> ping 2
    [ping esp]                 58
```
The ping duration of `58` vs `3` milliseconds is caused by using a `channel` (broadcast) instead of the Talker `name` (unicast) over WiFi, this represents a relative delay of 55 milliseconds just for using broadcast over WiFi.

## ESP32_EthernetENC_Arduino_Broadcast_SPI_Master
This example combines two Sockets in one Sketch. It shows the capacity of the board ESP32 to handle two SPI busses, the *HSPI* and *VSPI* busses, the ones that handle the Ethernet shield and the the Arduino boards connected to them respectively.

Because the SPI connections with the Arduinos are done via SPI Broadcast, you must add a resistor of **around 500 Ohms** to each Arduino SPI Slave MISO pin (D12), like so:
```
     [1st Slave Arduino MISO] ----[500Ω]----┐
     [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master ESP32 MISO]
     [3rd Slave Arduino MISO] ----[500Ω]----┘
```
Then, and only then, you can safely enable the SS pins for each Arduino board, like so:
```cpp
const int spi_pins[] = {4, 16};	// To which each Arduino CS (D10) pin is connected on the ESP32
auto& spi_socket = S_Broadcast_SPI_ESP_Arduino_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));
```
The existing *spy* Talker in this sketch lets you ping the existing local Talkers from the ESP32 board itself:
```
>>> list spy
    [call spy 0|ping]          Ping talkers by name or channel
    [call spy 1|ping_self]     I can even ping myself
    [call spy 2|call]          Able to do [<talker> <action>]
>>> call spy ping
    [call spy ping]            roger           0       test
    [call spy ping]            roger           1       blue
    [call spy ping]            roger           6       green
    [call spy ping]            roger           9       buzzer
    [call spy ping]            roger           12      yellow
>>> system blue board
    [system blue board]        ESP32 (Rev 100) (ID 00002C034DBF8473)
>>> system green board
    [system green board]       Arduino Uno/Nano (ATmega328P)
>>> system blue sockets
    [system blue sockets]      0       EthernetENC_Broadcast           10
    [system blue sockets]      1       Broadcast_SPI_ESP_Arduino_Master        20
>>> system green sockets
    [system green sockets]     0       Broadcast_SPI_Arduino_Slave     11
>>>
```
The last pair of numbers in front of each socket represent the link type, *up* or *down* for the first digit and the if *bridged*
for the second digit (Ex. 11 means uplinked and bridged).
## Mega_Ethernet
This is a simple sketch that implements the Ethernet Socket concerning the W5500 and W5100 shields.
It's targeted to the board Arduino Mega because it requires more memory than the other type of Sockets.

## Mega_EthernetENC
Also an Ethernet Socket, that is targeted to the ENC28J60 shield instead. It requires more
memory too, so, it should be used on high memory boards like the Arduino Mega or the ESP32.

## Nano_EtherCard
A low memory Ethernet Socket that is also targeted to the ENC28J60 shield, but by requiring
less memory, it can be used by the Arduino Uno or Nano. This one isn't able to communicate in
Unicast mode thought, so, it sends messages always in broadcast mode, meaning, it shouldn't
be used by WiFi connected devices given that WiFi delays or even drops broadcasted packages.

## Nano_Serial
Exemplifies a very simple type of Socket, the Serial one, so it requires very little to start working with,
on the other hand, it only works with two boards in a one-to-one connection, so, not a truly broadcast socket.

This is most useful for developing new Sockets or testing Manifestos given that you can connect to
boards right away via Serial and simulate that way more complex types of connectivity. See **Testing** bellow.

To connect the Board via Serial you start the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program like so:
```sh
python talk.py --socket SERIAL --port COM4
```

## Nano_Serial_Broadcast_SPI_Master
Besides implementing a Serial Socket, it also works as a SPI Master, this way it is possible to
command the SPI Master device via Serial in order to communicate with its multiple SPI Master devices,
transforming a non broadcasting Socket, the Serial one, into a broadcasting one the SPI.

## NanoBuzzer_Broadcast_SPI_Slave
A Sketch for the Arduino SPI Slave intended to be used together with a Sketch that implements the SPI Master socket like the `Broadcast_SPI_ESP_Arduino_Master` or
`ArduinoTestPair_Broadcast_SPI_2xArduino_Master`. This sketch controls a Buzzer on pin 2.

The SPI Slaves have their SPI Socket configured as *bridged*, so that it processes `local` messages too.
```cpp
	spi_socket.bridgeSocket();	// Makes sure it accepts LOCAL messages too
```

## NanoGreen_Broadcast_SPI_Slave
A Sketch for the Arduino SPI Slave intended to be used together with a Sketch that implements the SPI Master socket like the `Broadcast_SPI_ESP_Arduino_Master` or
`ArduinoTestPair_Broadcast_SPI_2xArduino_Master`. This sketch controls a Green led on pin 2 and an Yellow led on pin 19.

## Testing
These sketches are used to test Socket implementation that you can adapt to use for your own Socket implementations.

Because these sketches do SPI Broadcasting, the hardware shall have the MISO **500 Ohms** resistor on each SPI Slave board pin, like so:
```
     [1st Slave MISO] ----[500Ω]----┐
     [2nd Slave MISO] ----[500Ω]----┼---- [Master MISO]
     [3rd Slave MISO] ----[500Ω]----┘
```

All of the SPI Master examples bellow, also implement the Serial Socket, so that you can send commands to the *master* and the *slave* Talker,
concerning the SPI Master and the SPI Slave respectively.

To connect the Board via Serial you start the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program like so:
```sh
python talk.py --socket SERIAL --port COM4
```

Here is an example of the tests done on one of the SPI Sockets implementation via Serial.
```
>>> talk
    [talk master]              I'm the SPI Master
    [talk slave]               I'm the SPI Slave
>>> list master
    [call master 0|period]     Sets cycle period milliseconds
    [call master 1|enabled]    Checks, enable or disable cycles
    [call master 2|calls]      Gets total calls and their echoes
    [call master 3|burst]      Tests slave, many messages at once
    [call master 4|spacing]    Burst spacing in microseconds
    [call master 5|ping]       Ping talkers by name or channel
>>> call master ping
    [call master ping]         roger           2       slave
>>> call master calls
    [call master calls]        roger           669     669
>>> call master burst
    [call master burst]        roger           20      OK
>>> call master calls
    [call master calls]        roger           968     968
>>>
```
As you can see, the calls have the two values matching, meaning all calls are being successfully replied, and also, the burst testing passes,
meaning that all successive broadcasted messages are being received by the SPI Slave.

### ArduinoTestPair_Broadcast_SPI_2xArduino_Master
This Sketch concerns the testing of an Arduino SPI Master that is intended to control many Arduino SPI Slaves.

### ArduinoTestPair_Broadcast_SPI_Arduino_Slave
This Sketch concerns the testing of each Arduino SPI Slave controlled by the SPI Master referred above.

### ESP32TestPair_Broadcast_SPI_2xESP_4MHz_Master
This Sketch concerns the testing of an ESP32 SPI Master that is intended to control many ESP32 SPI Slaves.

Note that this Sketch is configured to use the **HSPI** bus. So, adapt it to your specific wiring.
```cpp
	// ================== INITIALIZE HSPI ==================
	// Initialize SPI with HSPI pins: MOSI=13, MISO=12, SCK=14
    spi_socket.begin(13, 12, 14);	// MOSI, MISO, SCK
```

### ESP32TestPair_Broadcast_SPI_2xESP_4MHz_Slave
This Sketch concerns the testing of each ESP32 SPI Slave controlled by the SPI Master referred above.

Note that this Sketch is configured to use the **VSPI** bus. So, adapt it to your specific wiring.
```cpp
	// ================== INITIALIZE VSPI ==================
	// Initialize SPI with VSPI pins: MOSI=23, MISO=19, SCK=18, CS=5
    spi_socket.begin(23, 19, 18, 5);	// MOSI, MISO, SCK, CS
```

### ESP32TestPair_Broadcast_SPI_ESP_Arduino_Master
This Sketch is homologous to the `ArduinoTestPair_Broadcast_SPI_2xArduino_Master` one, the only difference is that
it's intended to be uploaded to an ESP32 instead of an Arduino with all the rest being equal.

Note that this Sketch is configured to use the **HSPI** bus. So, adapt it to your specific wiring.
```cpp
	SPIClass* hspi = new SPIClass(HSPI);  // heap variable!
	// ================== INITIALIZE HSPI ==================
	// Initialize SPI with HSPI pins: SCK=14, MISO=12, MOSI=13, SS=15
	hspi->begin(14, 12, 13, 15);  // SCK, MISO, MOSI, SS
    spi_socket.begin(hspi);
```
