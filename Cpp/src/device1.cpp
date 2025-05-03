#include "json_device.h"
#include "json.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using json = nlohmann::json;

int main() {
    auto socket = std::make_shared<BroadcastSocket>(5005);
    JsonDevice device(socket, "PingDevice");
    
    device.setMessageCallback([&](const std::string& msg) {
        try {
            auto j = json::parse(msg);
            if (j["c"] == "pong") {
                std::cout << "✨ Received pong from " << j["sender"] << std::endl;
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
            json msg = {
                {"c", "ping"},
                {"sender", device.getName()},
                {"time", std::time(nullptr)}
            };
            device.sendJson(msg.dump());
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    } catch (...) {
        device.stop();
    }
    
    return 0;
}

