#[===[
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
]===]
cmake_minimum_required(VERSION 3.15)
project(JsonTalkie)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(Threads REQUIRED)

# Add nlohmann/json (header-only)
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Main executable
add_executable(device1 src/device1.cpp src/broadcast_socket.cpp src/json_device.cpp)
add_executable(device2 src/device2.cpp src/broadcast_socket.cpp src/json_device.cpp)

target_include_directories(device1 PRIVATE include)
target_include_directories(device2 PRIVATE include)
target_link_libraries(device1 PRIVATE Threads::Threads)
target_link_libraries(device2 PRIVATE Threads::Threads)