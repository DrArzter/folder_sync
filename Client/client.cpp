#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * This function creates a socket and connects to a server using TCP/IP.
 *
 * @return 0 if the connection is successful, 1 otherwise.
 *
 * @throws ErrorType if there is an error creating the socket, resolving the server's IP address,
 * connecting to the server, or receiving data from the server.
 */
int main() {
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

    if (getaddrinfo("127.0.0.1", nullptr, &hints, &res) != 0) {
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
