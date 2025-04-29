#ifndef BROADCAST_SOCKET_ETHERCARD_H
#define BROADCAST_SOCKET_ETHERCARD_H

#include "BroadcastSocket.h"
#include <EtherCard.h>

#define UDP_BUFFER_SIZE 128
#define ETHER_BUFFER_SIZE 500  // Typical size for ENC28J60

class BroadcastSocket_EtherCard : public BroadcastSocket {
public:
    explicit BroadcastSocket_EtherCard(uint16_t port = 5005);
    
    bool begin(uint8_t* mac, uint8_t csPin = 8);
    void end() override;
    bool write(const uint8_t* data, size_t length) override;
    bool available() override;
    size_t read(uint8_t* buffer, size_t size) override;
    
    const uint8_t* getSenderIP() const { return _remoteIp; }
    uint16_t getSenderPort() const { return _remotePort; }

    // Public callback handler
    void handlePacket(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len);

private:
    uint16_t _port;
    uint16_t _remotePort;
    uint8_t _remoteIp[4];
    uint8_t _recvBuffer[ETHER_BUFFER_SIZE];
    size_t _recvLength;
    
    // Static members for callback routing
    static BroadcastSocket_EtherCard* _activeInstance;
    static void _udpCallback(uint16_t dest_port, uint8_t* src_ip, uint16_t src_port, const char* data, uint16_t len);
};

#endif
