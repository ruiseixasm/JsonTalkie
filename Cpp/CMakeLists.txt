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