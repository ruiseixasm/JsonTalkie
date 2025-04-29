#ifndef BROADCAST_SOCKET_UDP_H
#define BROADCAST_SOCKET_UDP_H

#include "BroadcastSocket.h"
#include <Ethernet.h>      // For EthernetUDP and IPAddress
#include <EthernetUdp.h>   // For UDP functionality

class BroadcastSocket_UDP : public BroadcastSocket {
public:
    /**
     * @param port UDP port to use (default 5005)
     */
    explicit BroadcastSocket_UDP(uint16_t port = 5005);
    
    bool begin() override;
    void end() override;
    bool write(const uint8_t* data, size_t length) override;
    bool available() override;
    size_t read(uint8_t* buffer, size_t size) override;
    
    /**
     * Gets the sender IP of the last received packet
     */
    IPAddress getSenderIP() const { return _remoteIP; }
    
    /**
     * Gets the sender port of the last received packet
     */
    uint16_t getSenderPort() const { return _remotePort; }

private:
    EthernetUDP _udp;
    uint16_t _port;
    IPAddress _remoteIP;
    uint16_t _remotePort;
    bool _isInitialized = false;
};

#endif // BROADCAST_SOCKET_UDP_H
