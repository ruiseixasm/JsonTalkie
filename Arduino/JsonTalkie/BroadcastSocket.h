#ifndef BROADCAST_SOCKET_H
#define BROADCAST_SOCKET_H

// Arduino-compatible type definitions
typedef unsigned char uint8_t;
typedef unsigned int size_t;

class BroadcastSocket {
public:
    virtual ~BroadcastSocket() {}
    
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual bool write(const uint8_t* data, size_t length) = 0;
    virtual bool available() = 0;
    virtual size_t read(uint8_t* buffer, size_t size) = 0;
};


#endif
