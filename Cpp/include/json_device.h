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
#include "broadcast_socket.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

class JsonDevice {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    
    JsonDevice(std::shared_ptr<BroadcastSocket> socket, const std::string& name = "");
    ~JsonDevice();
    
    bool start();
    void stop();
    bool sendJson(const std::string& json);
    void setMessageCallback(MessageCallback callback);
    
private:
    void listenLoop();
    
    std::shared_ptr<BroadcastSocket> socket_;
    std::string device_id_;
    std::string name_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    MessageCallback callback_;
};

