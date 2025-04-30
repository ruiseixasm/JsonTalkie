/*
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
*/
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

