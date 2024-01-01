#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

struct ClientConfig {
    std::string serverIp;
    std::string folderPath;
};

/**
 * Reads the client configuration from the specified file.
 *
 * @param configFile the path to the configuration file
 *
 * @return the client configuration read from the file
 *
 * @throws std::ifstream::failure if the file cannot be opened
 */
ClientConfig readConfig(const std::string& configFile) {
    ClientConfig config;
    std::ifstream file(configFile);
    if (file.is_open()) {
        Json::Value root;
        file >> root;

        config.serverIp = root["serverIp"].asString();
        config.folderPath = root["folderPath"].asString();
    } else {
        // Provide default values if the file cannot be opened
        config.serverIp = "127.0.0.1";
        config.folderPath = "./default_sync_folder";  // Default folder in the current directory
        fs::create_directory(config.folderPath);      // Create the default folder
    }

    return config;
}

/**
 * This function creates a socket and connects to a server using TCP/IP.
 *
 * @param config the client configuration
 *
 * @return 0 if the connection is successful, 1 otherwise.
 *
 * @throws ErrorType if there is an error creating the socket, resolving the server's IP address,
 * connecting to the server, or receiving data from the server.
 */
int startClient(const ClientConfig& config) {

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // Connect to the server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345); // Change to the same port as the server

    // Resolve the server's IP address using getaddrinfo
    addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(config.serverIp.c_str(), nullptr, &hints, &res) != 0) {
        std::cerr << "Error resolving address." << std::endl;
        close(clientSocket);
        return 1;
    }

    memcpy(&serverAddress.sin_addr, &((sockaddr_in*)res->ai_addr)->sin_addr, sizeof(in_addr));
    freeaddrinfo(res);

    if (connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server." << std::endl;
        close(clientSocket);
        return 1;
    }

    // Send the folder path to the server
    send(clientSocket, config.folderPath.c_str(), config.folderPath.size(), 0);

    // Send last modification times of files to the server
    for (const auto& entry : fs::recursive_directory_iterator(config.folderPath)) {
        if (fs::is_regular_file(entry.path())) {
            auto lastModified = fs::last_write_time(entry.path());
            auto timePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(lastModified - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t time_tValue = std::chrono::system_clock::to_time_t(timePoint);

            send(clientSocket, &time_tValue, sizeof(time_tValue), 0);
        }
    }

    // Receive the synchronization result
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == -1) {
        std::cerr << "Error receiving data from the server." << std::endl;
    } else {
        buffer[bytesRead] = '\0';
        std::cout << "Server Response: " << buffer << std::endl;
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}

/**
 * Main function that runs the program.
 *
 * @param argc the number of command line arguments
 * @param argv an array of command line arguments
 *
 * @return 0 on success, 1 on failure
 *
 * @throws none
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::string configFile = argv[1];
    ClientConfig config = readConfig(configFile);

    while (true) {
        int result = startClient(config);
        if (result != 0) {
            // Handle errors if needed
            std::cerr << "Error in client synchronization." << std::endl;
        }

        // Sleep for a second before the next synchronization
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
