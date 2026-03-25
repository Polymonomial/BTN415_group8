#ifndef SENECA_SERVER_H
#define SENECA_SERVER_H

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <algorithm>

#include "devices/Device.h"
#include "devices/Light.h"
#include "devices/SecurityCamera.h"
#include "devices/Thermostat.h"
#include "network/ARPTable.h"
#include "network/RoutingTable.h"
#include "network/SubnetManager.h"

#include "../utils/SocketUtils.h" // Cross-platform socket includes

namespace seneca {

    class ClientHandler; // Forward declaration

    class TCPServer {
        socket_t server_socket;
        int next_client_id = 1;                // used in Step 2
        std::vector<ClientHandler*> client_list; // used in Step 3
        std::mutex mutex;                      // protects client_list
    public:
        TCPServer();
        ~TCPServer();

        void start(int port);          // Step 1: create socket, bind, listen
        void accept_clients();         // Step 2: accept connections
        void broadcast_message(ClientHandler* sender, const std::string& message); // Step 4
        void remove_client(ClientHandler* client); // Step 4
    };

    class ClientHandler {
    public:
        ClientHandler(socket_t sock, TCPServer* server, int id);
        ~ClientHandler();

        void run();       // Step 3: thread entry point
        void disconnect();

        // Accessors
        socket_t get_socket() const { return client_socket; }
        int get_id() const { return client_id; }

    private:
        socket_t client_socket;
        TCPServer* parent_server;
        int client_id;
    };

} // namespace seneca

#endif // SERVER_H