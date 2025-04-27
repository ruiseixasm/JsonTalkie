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

