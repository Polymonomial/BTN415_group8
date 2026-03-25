#include "utils/SocketSystem.h"

namespace seneca {

    SocketSystem::SocketSystem(){
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2,2),&wsaData);
        std::cout << "Window Socket - Initialized" << std::endl;
#endif
    }
    SocketSystem::~SocketSystem(){
#ifdef _WIN32
        WSACleanup();
#endif
    }


}