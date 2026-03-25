#include "server/Server.h"
#include <stdexcept>
#include <cstring> // for memset

namespace seneca {

    // ---------------- TCPServer ----------------
    TCPServer::TCPServer() {
        server_socket = INVALID_SOC;
    }

    TCPServer::~TCPServer() {
        if (server_socket != INVALID_SOC) {
            close_socket(server_socket);
        }
    }

    void TCPServer::start(int port) {
        std::cout << "Server yet to start" << std::endl;
        // TODO: Step 1
        // 1. Create socket
        // 2. Bind to port
        // 3. Listen
        socket_t server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock == INVALID_SOC) {
            throw std::runtime_error("Failed to create socket");
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close_socket(server_sock);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(server_sock, SOMAXCONN) < 0) {
            close_socket(server_sock);
            throw std::runtime_error("Failed to listen on socket");
        }

        server_socket = server_sock;
        std::cout << "Server started on port " << port << std::endl;
    }

    void TCPServer::accept_clients() {
        std::cout << "Server yet to accept clients" << std::endl;
        // TODO: Step 2
        while(true) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            socket_t client_sock = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
            if (client_sock == INVALID_SOC) {
                std::cerr << "Failed to accept client connection" << std::endl;
                continue; 
            }
            int client_id = next_client_id++;
            std::cout << "Client " << client_id << " connected from "
                      << inet_ntoa(client_addr.sin_addr) << ":"
                      << ntohs(client_addr.sin_port) << std::endl;
            ClientHandler* handler = new ClientHandler(client_sock, this, client_id);
            std::lock_guard<std::mutex> lock(mutex);
            client_list.push_back(handler);
            std::thread(&ClientHandler::run, handler).detach();
        }
        // Infinite loop to accept clients
        // Assign unique ID (next_client_id++)
        // Print connection
        // Placeholder for creating ClientHandler & spawning thread
        //    Step 3 TOD Task
        
        std::cout << "Server no longer accepting clients" << std::endl;
    }

    void TCPServer::broadcast_message(ClientHandler* sender, const std::string& message) {
        std::cout << "Server Yet to broadcast message" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        std::string labeled_message = "Message by [Client " + std::to_string(sender->get_id()) + "]: " + message;
        for (ClientHandler* client : client_list) {
            if (client != sender) {
                send(client->get_socket(), labeled_message.c_str(), labeled_message.size(), 0);
            }
            std::cout << "Server has now sent broadcast" << std::endl;
        }
        // TODO: Step 4
    }

    void TCPServer::remove_client(ClientHandler* client) {
        std::cout << "Server Yet to remove client" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        auto it = std::find_if(client_list.begin(), client_list.end(), [client](ClientHandler* c) { return c->get_id() == client->get_id(); });
        if (it != client_list.end()) {
            client_list.erase(it);
            std::cout << "A client is removed" << std::endl;
        }
        // TODO: Step 4
    }

    // ---------------- ClientHandler ----------------
    ClientHandler::ClientHandler(socket_t sock, TCPServer* server, int id)
        : client_socket(sock), parent_server(server), client_id(id) {}

    ClientHandler::~ClientHandler() {
        std::cout << "Client " << client_id << " disconnected." << std::endl;
    }

    void ClientHandler::run() {
        std::cout << "Client " << client_id
                << " is running on thread " 
                << std::this_thread::get_id() << std::endl;

        char buffer[1024];

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes <= 0) break;

            // Broadcast the message to all clients
            parent_server->broadcast_message(this, buffer);
        }

        disconnect();
    }

    void ClientHandler::disconnect() {
        std::cout << "Client beging disconnected" << std::endl;
        // TODO Step 4: remove client from server
        parent_server->remove_client(this);

        close_socket(client_socket);
        delete this;
    }

} // namespace seneca