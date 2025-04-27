#pragma once
#include <string>
#include <optional>
#include <sys/socket.h>
#include <netinet/in.h>

class BroadcastSocket {
public:
    BroadcastSocket(int port);
    ~BroadcastSocket();
    
    bool open();
    void close();
    bool send(const std::string& data);
    std::optional<std::pair<std::string, std::pair<std::string, int>>> receive();
    
private:
    int port_;
    int sockfd_ = -1;
    bool is_windows_;
};

