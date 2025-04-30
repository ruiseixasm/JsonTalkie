#include "BroadcastSocket_EtherCard.h"
uint8_t Ethernet::buffer[ETHER_BUFFER_SIZE]; // Essential for EtherCard

uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
const uint8_t CS_PIN = 8;

// MAC and CS pin in constructor
BroadcastSocket_EtherCard udp(5005, mymac, CS_PIN);

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial port
  
  Serial.println(F("\n[ENC28J60 UDP Broadcast]"));
  
  if (!udp.begin()) {
    Serial.println(F("Failed to initialize Ethernet"));
    while(1);
  }
  
  if (!ether.dhcpSetup()) {
    Serial.println(F("DHCP failed"));
    uint8_t staticip[] = {192,168,1,100};
    ether.staticSetup(staticip);
  }
  
  ether.printIp("IP: ", ether.myip);
  Serial.println(F("UDP broadcast ready"));
}

void loop() {
  ether.packetLoop(ether.packetReceive());
  
  static uint32_t lastBroadcast = 0;
  if (millis() - lastBroadcast >= 3000) {
    const char msg[] = "Hello from ENC28J60";
    if (udp.write((uint8_t*)msg, strlen(msg))) {
      Serial.println(F("Broadcast sent"));
    }
    lastBroadcast = millis();
  }
  
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
