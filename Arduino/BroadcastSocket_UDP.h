#ifndef UDP_BROADCAST_H
#define UDP_BROADCAST_H

#include "BroadcastSocket.h"
#include <EthernetUdp.h>

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

#endif
