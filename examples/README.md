# JsonTalkie - Talker Examples

Here is the description and purpose of the Examples above, where given [Sockets](https://github.com/ruiseixasm/JsonTalkie/tree/main/spckets)
and [Manifestos](https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos) are used.

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

in any case, the ping is clearly higher, given the low priority of broadcasted message over WiFi.
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

## Mega_Ethernet
...

## Mega_EthernetENC
...

## Nano_EtherCard
...

## Nano_Serial
...

## Nano_Serial_Broadcast_SPI_Master
...

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



