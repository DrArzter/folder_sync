#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

/**
 * Synchronizes the folders from the source directory to the destination directory.
 *
 * @param sourceDir the path to the source directory
 * @param destDir the path to the destination directory
 * @param clientSocket the client socket used for communication
 *
 * @throws std::exception if an error occurs during synchronization
 */
void synchronizeFolders(const std::string& sourceDir, const std::string& destDir, int clientSocket) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(sourceDir)) {
            fs::path relativePath = fs::relative(entry.path(), sourceDir);
            fs::path destPath = destDir / relativePath;

            if (fs::is_directory(entry.path())) {
                // Create directory in destination if it doesn't exist
                fs::create_directories(destPath);
            } else if (fs::is_regular_file(entry.path())) {
                // Check if the file exists in the destination folder
                if (fs::exists(destPath)) {
                    // Compare modification times to determine if the file needs to be updated
                    auto srcTime = fs::last_write_time(entry.path());
                    auto destTime = fs::last_write_time(destPath);
                    if (srcTime > destTime) {
                        // Copy the file if it's newer in the source folder
                        fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                    }
                } else {
                    // File doesn't exist in destination, so copy it
                    fs::copy_file(entry.path(), destPath);
                }
            }
        }

        // Send a response to the client indicating the synchronization is complete
        const char* response = "Synchronization complete.";
        send(clientSocket, response, strlen(response), 0);
    } catch (const std::exception& e) {
        // Handle exceptions and send an error response to the client
        std::cerr << "Error during synchronization: " << e.what() << std::endl;
        const char* response = "Error during synchronization.";
        send(clientSocket, response, strlen(response), 0);
    }
}

/**
 * Main function that creates a socket, binds it to a specific IP and port, listens for incoming connections,
 * and handles synchronization for each connection in a separate thread or process.
 *
 * @return 0 on successful execution.
 *
 * @throws ErrorType if there is an error creating the socket, binding the socket, listening for connections,
 * accepting a connection, or handling synchronization.
 */
int main() {
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // Bind the socket to a specific IP and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(12345); // Change to the desired port number

    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket." << std::endl;
        close(serverSocket);
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening for connections." << std::endl;
        close(serverSocket);
        return 1;
    }

    while (true) {
        // Accept a connection
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            std::cerr << "Error accepting connection." << std::endl;
            close(serverSocket);
            return 1;
        }

        // Handle synchronization for each connection in a separate thread or process
        // For simplicity, we'll handle synchronization in the same thread here
        synchronizeFolders("/path/to/source", "/path/to/destination", clientSocket);

        // Close the client socket
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
