cmake_minimum_required(VERSION 3.16.0)

# Build For ARM
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Start Project
project(inventorysaving)

# Include SDK
include("$ENV{HOME}/.minecraft-pi/sdk/lib/minecraft-pi-reborn-client/sdk/sdk.cmake")

# Build
add_library(inventorysaving SHARED inventorysaving.cpp)
target_link_libraries(inventorysaving mods-headers reborn-patch symbols misc)
