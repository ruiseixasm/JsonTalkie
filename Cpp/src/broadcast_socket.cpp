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
#include "broadcast_socket.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <cstring>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

BroadcastSocket::BroadcastSocket(int port) : port_(port) {
    #ifdef _WIN32
    is_windows_ = true;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
    #else
    is_windows_ = false;
    #endif
}

BroadcastSocket::~BroadcastSocket() {
    close();
    #ifdef _WIN32
    WSACleanup();
    #endif
}

bool BroadcastSocket::open() {
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) return false;

    int optval = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    setsockopt(sockfd_, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close();
        return false;
    }

    // Set non-blocking
    #ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(sockfd_, FIONBIO, &mode);
    #else
    fcntl(sockfd_, F_SETFL, O_NONBLOCK);
    #endif

    return true;
}

void BroadcastSocket::close() {
    if (sockfd_ >= 0) {
        #ifdef _WIN32
        closesocket(sockfd_);
        #else
        ::close(sockfd_);
        #endif
        sockfd_ = -1;
    }
}

bool BroadcastSocket::send(const std::string& data) {
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port_);
    dest.sin_addr.s_addr = is_windows_ ? inet_addr("127.255.255.255") : INADDR_BROADCAST;

    return sendto(sockfd_, data.c_str(), data.size(), 0, 
                (sockaddr*)&dest, sizeof(dest)) >= 0;
}

std::optional<std::pair<std::string, std::pair<std::string, int>>> BroadcastSocket::receive() {
    char buffer[4096];
    sockaddr_in src{};
    socklen_t srclen = sizeof(src);

    int len = recvfrom(sockfd_, buffer, sizeof(buffer), 0, (sockaddr*)&src, &srclen);
    if (len <= 0) return std::nullopt;

    return std::make_pair(
        std::string(buffer, len),
        std::make_pair(inet_ntoa(src.sin_addr), ntohs(src.sin_port))
    );
}
