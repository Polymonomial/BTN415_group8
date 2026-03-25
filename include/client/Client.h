#ifndef SENECA_CLIENT_H
#define SENECA_CLIENT_H

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <stdexcept>
#include "../utils/SocketUtils.h"  // Cross-platform socket includes

namespace seneca {

    class TCPClient {
    public:
        TCPClient();
        ~TCPClient();

        void connect_to_server(const std::string& ip, int port);
        void start();  // Starts sending/receiving loop

    private:
        socket_t client_socket;
        std::mutex io_mutex;  // Protects console output
        void receive_loop();  // Thread for receiving messages

        void close_connection();
    };

} // namespace seneca

#endif // SENECA_CLIENT_H