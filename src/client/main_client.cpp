#include "client/Client.h"
#include "utils/SocketSystem.h"  // For Winsock init/cleanup
#include <iostream>

using namespace seneca;
using namespace std;

int main() {
    try {
        SocketSystem ss; // Initializes Winsock on Windows

        TCPClient client;

        string server_ip;
        int server_port;

        cout << "Enter server IP (e.g., 127.0.0.1): ";
        cin >> server_ip;
        cout << "Enter server port (e.g., 5000): ";
        cin >> server_port;
        cin.ignore(); // Ignore leftover newline

        client.connect_to_server(server_ip, server_port);
        client.start();  // Blocks until user quits (q)
    }
    catch (const std::exception& ex) {
        cerr << "Error: " << ex.what() << endl;
        return 1;
    }

    return 0;
}