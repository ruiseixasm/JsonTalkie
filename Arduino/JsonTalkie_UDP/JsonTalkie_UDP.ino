/*
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
*/
#include "BroadcastSocket_UDP.h"

// Network configuration
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
BroadcastSocket_UDP udp(5005);  // Port set in constructor

void setup() {
  Serial.begin(9600);
  while (!Serial);  // Wait for serial port on Leonardo
  
  // Start Ethernet connection
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while (true);  // Halt on failure
  }
  
  // Initialize UDP
  if (!udp.begin()) {
    Serial.println("Failed to start UDP socket");
    while (true);
  }
  
  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("UDP broadcast ready");
}

void loop() {
  // 1. Broadcast a message every 3 seconds
  static unsigned long lastBroadcast = 0;
  if (millis() - lastBroadcast >= 3000) {
    const char message[] = "Arduino Broadcast";
    if (udp.write((const uint8_t*)message, strlen(message))) {
      Serial.println("Broadcast sent");
    } else {
      Serial.println("Broadcast failed");
    }
    lastBroadcast = millis();
  }
  
  // 2. Check for incoming messages
  if (udp.available()) {
    uint8_t buffer[128];
    size_t bytesRead = udp.read(buffer, sizeof(buffer));
    
    Serial.print("Received from ");
    Serial.print(udp.getSenderIP());
    Serial.print(":");
    Serial.print(udp.getSenderPort());
    Serial.print(" - ");
    Serial.write(buffer, bytesRead);
    Serial.println();
  }
  
  // 3. Maintain DHCP lease (important for long-running sketches)
  Ethernet.maintain();
}
