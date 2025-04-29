#ifndef BROADCAST_SOCKET_H
#define BROADCAST_SOCKET_H

#include <Arduino.h>
#include <cstddef> // for size_t
#include <cstdint> // for uint8_t

class BroadcastSocket {
public:
    virtual ~BroadcastSocket() = default;
    
    /**
     * Initialize communication
     * @return true if successful
     */
    virtual bool begin() = 0;
    
    /**
     * Release communication resources
     */
    virtual void end() = 0;
    
    /**
     * Send raw data
     * @param data Pointer to byte buffer
     * @param length Number of bytes to send
     * @return true if successful
     */
    virtual bool write(const uint8_t* data, size_t length) = 0;
    
    /**
     * Check for available data
     * @return true if data is available to read
     */
    virtual bool available() = 0;
    
    /**
     * Read received data
     * @param buffer Where to store the data
     * @param size Maximum bytes to read
     * @return Number of bytes actually read
     */
    virtual size_t read(uint8_t* buffer, size_t size) = 0;
};

#endif
