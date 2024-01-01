#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <set>
#include <ctime> // Include ctime for difftime
#include <filesystem>

namespace fs = std::filesystem;

#define MAX_PATH_LEN 1024
#define MAX_BUFFER_SIZE 1024

struct ServerConfig {
    std::string serverSyncFolder;
    int port;
};

/**
 * Reads the server configuration from the specified file.
 *
 * @param configFile the path to the configuration file
 *
 * @return the server configuration read from the file
 *
 * @throws std::ifstream::failure if the file cannot be opened
 */
ServerConfig readConfig(const std::string& configFile) {
    ServerConfig config;
    std::ifstream file(configFile);
    if (file.is_open()) {
        Json::Value root;
        file >> root;

        if (root.isMember("serverSyncFolder")) {
            config.serverSyncFolder = root["serverSyncFolder"].asString();
        } else {
            // Create a default folder in the current directory
            config.serverSyncFolder = "./default_sync_folder";
            mkdir(config.serverSyncFolder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }

        config.port = root.isMember("port") ? root["port"].asInt() : 12345;
    } else {
        // Provide default values if the file cannot be opened
        config.serverSyncFolder = "./default_sync_folder";
        mkdir(config.serverSyncFolder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        config.port = 12345;
    }

    return config;
}


/**
 * Synchronizes the folders between a client and a server.
 *
 * @param clientSourceDir The path to the client source folder.
 * @param serverDestDir The path to the server destination folder.
 * @param clientSocket The socket of the client.
 *
 * @throws Error getting file information if there is an error while getting information about a file.
 * @throws Error writing to destination file if there is an error while writing to the destination file.
 * @throws Error getting server file information if there is an error while getting information about a server file.
 * @throws Error writing to client destination file if there is an error while writing to the client destination file.
 */
void synchronizeFolders(const char* clientSourceDir, const char* serverDestDir, int clientSocket) {

    // Sync from client to server
    DIR* dir;
    struct dirent* entry;

    dir = opendir(clientSourceDir);
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            char relativePath[MAX_PATH_LEN];
            snprintf(relativePath, sizeof(relativePath), "%s/%s", clientSourceDir, entry->d_name);

            struct stat fileStat;
            if (stat(relativePath, &fileStat) == -1) {
                perror("Error getting file information");
                continue;
            }

            if (S_ISDIR(fileStat.st_mode)) {
                // Create directory in destination if it doesn't exist
                char destPath[MAX_PATH_LEN];
                snprintf(destPath, sizeof(destPath), "%s/%s", serverDestDir, entry->d_name);
                mkdir(destPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            } else if (S_ISREG(fileStat.st_mode)) {
                // Compare modification times to determine if the file needs to be updated
                char destPath[MAX_PATH_LEN];
                snprintf(destPath, sizeof(destPath), "%s/%s", serverDestDir, entry->d_name);

                int srcFile = open(relativePath, O_RDONLY);
                int destFile = open(destPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if (srcFile != -1 && destFile != -1) {
                    struct stat srcStat, destStat;
                    fstat(srcFile, &srcStat);
                    fstat(destFile, &destStat);

                    if (difftime(srcStat.st_mtime, destStat.st_mtime) > 0) {
                        // Copy the file if it's newer in the source folder
                        char buffer[MAX_BUFFER_SIZE];
                        ssize_t bytesRead, bytesWritten;

                        while ((bytesRead = read(srcFile, buffer, sizeof(buffer))) > 0) {
                            bytesWritten = write(destFile, buffer, bytesRead);
                            if (bytesWritten == -1) {
                                perror("Error writing to destination file");
                                break;
                            }
                        }
                    }

                    close(destFile);
                }

                if (srcFile != -1) {
                    close(srcFile);
                }
            }
        }

        closedir(dir);
    }

    // Sync from server to client
    DIR* serverDir;
    struct dirent* serverEntry;

    serverDir = opendir(serverDestDir);
    if (serverDir != NULL) {
        while ((serverEntry = readdir(serverDir)) != NULL) {
            if (strcmp(serverEntry->d_name, ".") == 0 || strcmp(serverEntry->d_name, "..") == 0) {
                continue; // Skip "." and ".."
            }

            char serverRelativePath[MAX_PATH_LEN];
            snprintf(serverRelativePath, sizeof(serverRelativePath), "%s/%s", serverDestDir, serverEntry->d_name);

            struct stat serverFileStat;
            if (stat(serverRelativePath, &serverFileStat) == -1) {
                perror("Error getting server file information");
                continue;
            }

            if (S_ISDIR(serverFileStat.st_mode)) {
                // Create directory in client if it doesn't exist
                char clientDestPath[MAX_PATH_LEN];
                snprintf(clientDestPath, sizeof(clientDestPath), "%s/%s", clientSourceDir, serverEntry->d_name);
                mkdir(clientDestPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            } else if (S_ISREG(serverFileStat.st_mode)) {
                // Compare modification times to determine if the file needs to be updated
                char clientDestPath[MAX_PATH_LEN];
                snprintf(clientDestPath, sizeof(clientDestPath), "%s/%s", clientSourceDir, serverEntry->d_name);

                int serverFile = open(serverRelativePath, O_RDONLY);
                int clientDestFile = open(clientDestPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if (serverFile != -1 && clientDestFile != -1) {
                    struct stat serverStat, clientDestStat;
                    fstat(serverFile, &serverStat);
                    fstat(clientDestFile, &clientDestStat);

                    if (difftime(serverStat.st_mtime, clientDestStat.st_mtime) > 0) {
                        // Copy the file if it's newer on the server
                        char buffer[MAX_BUFFER_SIZE];
                        ssize_t bytesRead, bytesWritten;

                        while ((bytesRead = read(serverFile, buffer, sizeof(buffer))) > 0) {
                            bytesWritten = write(clientDestFile, buffer, bytesRead);
                            if (bytesWritten == -1) {
                                perror("Error writing to client destination file");
                                break;
                            }
                        }
                    }

                    close(clientDestFile);
                }

                if (serverFile != -1) {
                    close(serverFile);
                }
            }
        }

        closedir(serverDir);
    }

    // Send a response to the client indicating the synchronization is complete
    const char* response = "Bidirectional Synchronization complete.";
    send(clientSocket, response, strlen(response), 0);

    // Print to console
    std::cout << "Bidirectional Synchronization complete for folder: " << clientSourceDir << std::endl;
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
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::string configFile = argv[1];
    ServerConfig config = readConfig(configFile);

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
    serverAddress.sin_port = htons(config.port);

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

        // Receive the client's folder path
        char folderPathBuffer[MAX_PATH_LEN];
        ssize_t pathBytesRead = recv(clientSocket, folderPathBuffer, sizeof(folderPathBuffer), 0);
        if (pathBytesRead == -1) {
            std::cerr << "Error receiving folder path from client." << std::endl;
            close(clientSocket);
            continue;
        }
        folderPathBuffer[pathBytesRead] = '\0';

        // Handle bidirectional synchronization for each connection
        synchronizeFolders(folderPathBuffer, config.serverSyncFolder.c_str(), clientSocket);

        // Close the client socket
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}