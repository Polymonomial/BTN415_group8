#include "client/Client.h"
#include <cstring>   // memset
#include <stdexcept> // std::runtime_error

namespace seneca {

// ---------------- Constructor / Destructor ----------------
TCPClient::TCPClient() : client_socket(INVALID_SOC) {}

TCPClient::~TCPClient() {
    close_connection();
}

// ---------------- Connect to Server ----------------
void TCPClient::connect_to_server(const std::string& ip, int port) {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOC) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close_connection();
        throw std::runtime_error("Failed to connect to server");
    }

    std::cout << "Connected to server " << ip << ":" << port << std::endl;
}

// ---------------- Start Sending/Receiving ----------------
void TCPClient::start() {
    // Launch receive thread
    std::thread(&TCPClient::receive_loop, this).detach();

    std::string input;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "> ";
            std::cout.flush();
        }

        std::getline(std::cin, input);
        if (input == "q") break;

        int sent = send(client_socket, input.c_str(), input.size(), 0);
        if (sent <= 0) {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Failed to send message. Disconnecting..." << std::endl;
            break;
        }
    }

    close_connection();
}

// ---------------- Receive Loop ----------------
void TCPClient::receive_loop() {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "\n" << buffer << "\n> ";
        std::cout.flush();
    }
}

// ---------------- Close Connection ----------------
void TCPClient::close_connection() {
    if (client_socket != INVALID_SOC) {
        close_socket(client_socket);
        client_socket = INVALID_SOC;
    }
}

} // namespace seneca