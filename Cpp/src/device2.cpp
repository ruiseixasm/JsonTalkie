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
                
                json response = {
                    {"type", "pong"},
                    {"sender", device.getName()},
                    {"original_time", j["time"]}
                };
                device.sendJson(response.dump());
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

