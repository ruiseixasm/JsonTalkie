#ifndef BROADCAST_SOCKET_UDP_H
#define BROADCAST_SOCKET_UDP_H

#include "BroadcastSocket.h"
#include <Ethernet.h>
#include <EthernetUdp.h>

class BroadcastSocket_UDP : public BroadcastSocket {
public:
    explicit BroadcastSocket_UDP(uint16_t port = 5005); // Port set here
    
    bool begin() override; // No parameters
    void end() override;
    bool write(const uint8_t* data, size_t length) override;
    bool available() override;
    size_t read(uint8_t* buffer, size_t size) override;
    
    IPAddress getSenderIP() const { return _remoteIP; }
    uint16_t getSenderPort() const { return _remotePort; }

private:
    EthernetUDP _udp;
    uint16_t _port;
    IPAddress _remoteIP;
    uint16_t _remotePort;
    bool _isInitialized = false;
};

#endif // BROADCAST_SOCKET_UDP_H
