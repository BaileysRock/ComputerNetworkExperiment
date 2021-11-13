cmake_minimum_required(VERSION 3.17)
project(Lab3_GBNProtocol)

set(CMAKE_CXX_STANDARD 14)

link_libraries(ws2_32 wsock32)
add_executable(Server Server.cpp)
add_executable(Client Client.cpp)

add_executable(ServerSR ServerSR.cpp)
add_executable(ClientSR ClientSR.cpp)

