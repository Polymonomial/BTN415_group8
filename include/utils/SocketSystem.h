#ifndef SENECA_SOCKET_SYSTEM_H
#define SENECA_SOCKET_SYSTEM_H

#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib,"ws2_32.lib")
#endif 

namespace seneca {
    class SocketSystem {
        public:
            SocketSystem();
            ~SocketSystem();
    };
}

#endif 