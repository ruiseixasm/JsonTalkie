#include "BroadcastSocket_EtherCard.h"

// ENC28J60 configuration
uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
uint8_t myip[] = { 192,168,1,100 }; // Fallback IP
const int CS_PIN = 10; // Chip Select pin

BroadcastSocket_EtherCard udp(5005);

void setup() {
  Serial.begin(9600);
  Serial.println(F("\n[ENC28J60 UDP Broadcast]"));
  
  // Initialize Ethernet
  if (ether.begin(ETHER_BUFFER_SIZE, mymac, CS_PIN)) {
    Serial.println(F("Failed to access Ethernet controller"));
    while(1);
  }
  
  // Network configuration
  if (!ether.dhcpSetup()) {
    Serial.println(F("DHCP failed, using static IP"));
    ether.staticSetup(myip);
  }
  
  // Start UDP
  if (!udp.begin()) {
    Serial.println(F("Failed to start UDP"));
    while(1);
  }
  
  ether.printIp("IP: ", ether.myip);
  Serial.println(F("UDP broadcast ready"));
}

void loop() {
  // Process incoming packets
  ether.packetLoop(ether.packetReceive());
  
  // Broadcast every 3 seconds
  static uint32_t lastBroadcast = 0;
  if (millis() - lastBroadcast >= 3000) {
    const char msg[] = "Hello from ENC28J60";
    if (udp.write((uint8_t*)msg, strlen(msg))) {
      Serial.println(F("Broadcast sent"));
    }
    lastBroadcast = millis();
  }
  
  // Handle received packets
  if (udp.available()) {
    uint8_t buffer[UDP_BUFFER_SIZE];
    size_t len = udp.read(buffer, sizeof(buffer));
    
    Serial.print(F("Received from "));
    ether.printIp(udp.getSenderIP());
    Serial.print(F(":"));
    Serial.print(udp.getSenderPort());
    Serial.print(F(" - "));
    Serial.write(buffer, len);
    Serial.println();
  }
}
