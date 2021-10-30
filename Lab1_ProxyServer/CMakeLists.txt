cmake_minimum_required(VERSION 3.17)
project(Lab1_ProxyServer)

set(CMAKE_CXX_STANDARD 14)
link_libraries(ws2_32 wsock32)

aux_source_directory(${PROJECT_SOURCE_DIR} DIR_MAIN_SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/Class DIR_HELLO_SRCS)



#add_executable(HttpProxyServer HttpProxyServer.cpp)
add_executable(HttpProxyServerRun ${DIR_MAIN_SRCS} ${DIR_HELLO_SRCS} ${DIR_WORLD_SRCS})