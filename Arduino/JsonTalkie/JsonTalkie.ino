#include "BroadcastSocket_UDP.h"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
BroadcastSocket_UDP udp;

void setup() {
  Ethernet.begin(mac);
  udp.begin(5005); // Initialize with port
}

void loop() {
  // Broadcast message
  const char message[] = "Hello World";
  udp.write((uint8_t*)message, strlen(message));
  
  // Check for responses
  if (udp.available()) {
    uint8_t buffer[128];
    size_t len = udp.read(buffer, sizeof(buffer));
    
    Serial.print("Received from ");
    Serial.print(udp.getSenderIP());
    Serial.print(":");
    Serial.print(udp.getSenderPort());
    Serial.print(" - ");
    Serial.write(buffer, len);
    Serial.println();
  }
  
  delay(1000);
}
