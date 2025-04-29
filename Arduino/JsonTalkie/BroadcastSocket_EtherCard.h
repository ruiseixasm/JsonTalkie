#ifndef BROADCAST_SOCKET_ETHERCARD_H
#define BROADCAST_SOCKET_ETHERCARD_H

#include "BroadcastSocket.h"
#include <EtherCard.h>

#define UDP_BUFFER_SIZE 128
#define ETHER_BUFFER_SIZE 500  // Typical size for ENC28J60

class BroadcastSocket_EtherCard : public BroadcastSocket {
public:
    BroadcastSocket_EtherCard(uint16_t port, uint8_t* mac, uint8_t csPin = 8);
    
    bool begin() override;
    void end() override;
    bool write(const uint8_t* data, size_t length) override;
    bool available() override;
    size_t read(uint8_t* buffer, size_t size) override;
    
    const uint8_t* getSenderIP() const { return _remoteIp; }
    uint16_t getSenderPort() const { return _remotePort; }

private:
    uint16_t _port;
    uint8_t* _mac;
    uint8_t _csPin;
    uint16_t _remotePort;
    uint8_t _remoteIp[4];
    uint8_t _recvBuffer[UDP_BUFFER_SIZE];
    size_t _recvLength;
    
    void handlePacket(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len);
    static void udpCallback(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len);
    static BroadcastSocket_EtherCard* _instance;
};

#endif
