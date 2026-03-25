#include <iostream>
#include "utils/SocketSystem.h"
#include "server/Server.h"


using namespace seneca;


int main() {
    SocketSystem ss;
    
    try{
        TCPServer server;
        server.start(5000);
        server.accept_clients();
    }catch(std::exception& e){
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}