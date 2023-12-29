cmake_minimum_required(VERSION 3.10)
project(FolderSync)

set(CMAKE_CXX_STANDARD 17)

# Include the directory containing common headers if any
include_directories(include)

# Find JSONCPP package
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Server executable
add_executable(server Server/server.cpp)
target_include_directories(server PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(server PRIVATE ${JSONCPP_LIBRARIES})

# Client executable
add_executable(client Client/client.cpp)
target_include_directories(client PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(client PRIVATE ${JSONCPP_LIBRARIES})

# Copy config.json to build directory with modified names for both server and client
configure_file(Server/config.json ${CMAKE_CURRENT_BINARY_DIR}/server_config.json COPYONLY)
configure_file(Client/config.json ${CMAKE_CURRENT_BINARY_DIR}/client_config.json COPYONLY)
