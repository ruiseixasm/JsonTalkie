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

