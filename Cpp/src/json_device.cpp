#include "json_device.h"
#include <iostream>

JsonDevice::JsonDevice(std::shared_ptr<BroadcastSocket> socket, const std::string& name)
    : socket_(socket), name_(name.empty() ? "Device-" + device_id_.substr(0, 8) : name) {}

JsonDevice::~JsonDevice() {
    stop();
}

bool JsonDevice::start() {
    if (!socket_->open()) return false;
    running_ = true;
    thread_ = std::thread(&JsonDevice::listenLoop, this);
    return true;
}

void JsonDevice::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
    socket_->close();
}

bool JsonDevice::sendJson(const std::string& json) {
    return socket_->send(json);
}

void JsonDevice::setMessageCallback(MessageCallback callback) {
    callback_ = callback;
}

void JsonDevice::listenLoop() {
    while (running_) {
        auto received = socket_->receive();
        if (received && callback_) {
            callback_(received->first);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

