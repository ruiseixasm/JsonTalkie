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
#include "json_device.h"
#include "json.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using json = nlohmann::json;

int main() {
    auto socket = std::make_shared<BroadcastSocket>(5005);
    JsonDevice device(socket, "PongDevice");
    
    device.setMessageCallback([&](const std::string& msg) {
        try {
            auto j = json::parse(msg);
            if (j["type"] == "ping") {
                std::cout << "🏓 Received ping from " << j["sender"] << std::endl;
                
                json reply = {
                    {"type", "pong"},
                    {"sender", device.getName()},
                    {"original_time", j["time"]}
                };
                device.sendJson(reply.dump());
            }
        } catch (...) {
            std::cerr << "Invalid JSON received" << std::endl;
        }
    });
    
    if (!device.start()) {
        std::cerr << "Failed to start device" << std::endl;
        return 1;
    }
    
    try {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (...) {
        device.stop();
    }
    
    return 0;
}

