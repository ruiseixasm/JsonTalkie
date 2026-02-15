# JsonTalkie - Talker Examples

Here is the description and purpose of the Examples above, where given [Sockets](https://github.com/ruiseixasm/JsonTalkie/tree/main/spckets)
and [Manifestos](https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos) are used.

The command line bellow is from the [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy) program.

## ESP_WiFi
This is an Example where the WiFi is used as the Socket interface to the Talker. By being an WiFi Socket it has limitations concerning the Broadcast usage, because WiFi has a limit of Broadcasted packages, this means that if oyu send too many Broadcasted commands they will start to drop. In order to send NON Broadcasted commands make sure you add the name of the Talker in each command, like so:
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
As you can see above, by always making sure the name is used, no drops occurred, on the other hand, by using a broadcast message, like in the case of a channel, those drops may start to happen, due to the WiFi Broadcast limitations.

In any case, the ping is clearly higher, given the low priority of broadcasted message over WiFi.
```
>>> channel esp
    [channel esp]              255
>>> channel esp 2
    [channel esp]              2
>>> ping 2
    [ping esp]                 58
```
The delay of `58` vs `3` milliseconds is caused by using a `channel` (broadcast) instead of the Talker `name` (unicast) over WiFi.

## ESP32_EthernetENC_Arduino_Broadcast_SPI_Master
This example combines two Sockets in one Sketch. It shows the capacity of the board ESP32 handle two SPI busses, *HSPI* and *VSPI*, to handle the Ethernet shield and the the Arduino boards connected to it, respectively.

Because the SPI connection with the Arduinos is done via SPI Broadcast, you must add a resistor of **around 500 Ohms** to each SPI Slave MISO pin, like so:
```
     [1st Slave Arduino MISO] ----[500Ω]----┐
     [2nd Slave Arduino MISO] ----[500Ω]----┼---- [Master Arduino MISO]
     [3rd Slave Arduino MISO] ----[500Ω]----┘
```
Then, and only then, you can safely enable the SS pins for each Arduino, like so:
```cpp
const int spi_pins[] = {4, 16};	// To which each Arduino CS pin is connected on the ESP32
auto& spi_socket = S_Broadcast_SPI_ESP_Arduino_Master::instance(spi_pins, sizeof(spi_pins)/sizeof(int));
```
The existing `spy` Talker in this sketch lets you ping the existing local Talkers from the ESP32 board itself:
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
The last numbers in front of each socket, means the link type, *up* or *down*, and the *bridged* condition (Ex. 11 means uplinked and bridged).
## Mega_Ethernet
This is a simple sketch that implements the Ethernet Socket concerning the W5500 or W5100 shield.
It's targeted to the board Arduino Mega because it requires more memory than other type of Sockets.

## Mega_EthernetENC
Also an Ethernet Socket, that is targeted to the ENC28J60 shield instead. It requires more
memory than other Sockets, so, it should be used in high memory boards like the Arduino Mega.

## Nano_EtherCard
A low memory Ethernet Socket that is also targeted to the ENC28J60 shield, but by requiring
less memory, it can be used by the Arduino Uno or Nano. This one isn't able to communicate in
Unicast mode thought, so, it sends messages always in broadcast mode, meaning, it shouldn't
be used by WiFi connected devices given that WiFi may drop this types of packages.

## Nano_Serial
Exemplifies a very simple type of Socket, the Serial communication one, so it requires very little to start working, on the other hand, it only works with a single slave, so, not a
truly broadcast socket.

## Nano_Serial_Broadcast_SPI_Master
Besides implementing a Serial Socket, it also works as a SPI Master, this way it is possible to
use command the SPI Master device via Serial in order to communicate with its multiple SPI Master devices, transforming a non broadcasting Socket, the Serial one, into a broadcast
communication the SPI one.


## NanoBuzzer_Broadcast_SPI_Slave
...

## NanoGreen_Broadcast_SPI_Slave
...

## Testing
...

### ArduinoTestPair_Broadcast_SPI_2xArduino_Master
...

### ArduinoTestPair_Broadcast_SPI_Arduino_Slave
...

### ESP32TestPair_Broadcast_SPI_2xESP_128Bytes_Master
...

### ESP32TestPair_Broadcast_SPI_2xESP_128Bytes_Slave
...

### ESP32TestPair_Broadcast_SPI_ESP_Arduino_Master
...



