
BTN415 – Data Communication Programming
Smart Home Network Automation System
Network Architecture Report
Group	Group 8
Course	BTN415 – Data Communication Programming
Section	Network Design & Architecture


 
1.  Network Architecture Overview
The Smart Home Network Automation System uses a TCP/IP network infrastructure to connect multiple smart devices across logically segmented subnets. The network is designed using Variable-Length Subnet Masking (VLSM) to allocate address space efficiently, with dedicated subnets for each device category. A simulated routing layer, ARP cache, and IP address manager work together to ensure that client applications can discover and control devices regardless of which subnet they reside on.

The four core networking components are:
•	 – allocates and tracks IP addresses within each named subnet
•	 – implements longest-prefix-match routing between subnets
•	 – resolves IP addresses to MAC addresses for local delivery
•	 – defines the base network, per-subnet parameters, and bitwise utility functions

Base Network
Address space:  192.168.1.0 / 24
Total hosts:    254  (192.168.1.1 – 192.168.1.254)
Split into 4 VLSM subnets covering device categories: Lighting, Thermostat, Security, Management

2.  VLSM Subnet Design
VLSM allows each subnet to be sized exactly to its requirements instead of wasting a uniform block on every segment. The base 192.168.1.0/24 space is divided into four subnets of decreasing size, listed below from largest to smallest.

Subnet name	Network address	Subnet mask	CIDR	Max hosts	Gateway	Usage
Lighting	192.168.1.0	255.255.255.192	/26	62	192.168.1.1	Smart lights
Thermostat	192.168.1.64	255.255.255.224	/27	30	192.168.1.65	Thermostats
Security	192.168.1.96	255.255.255.224	/27	30	192.168.1.97	Security cameras
Management	192.168.1.128	255.255.255.240	/28	14	192.168.1.129	Admin / server

2.1  Why these sizes?
Lighting is allocated a /26 (62 hosts) because a smart home could have dozens of individually addressable lights. Thermostat and Security both use /27 (30 hosts), which comfortably covers all sensor and camera nodes. Management uses the smallest /28 (14 hosts) since it only needs addresses for the server itself and a handful of administrative interfaces.

2.2  IP allocation logic
SubnetManager.cpp initialises the four subnets from NetworkConfig::getSubnetConfigurations() and tracks allocations per subnet. Host numbering starts at 2 (host 1 is always reserved for the gateway). The allocation algorithm is:

uint32_t netAddr = NetworkConfig::ipToUint32(subnet.networkAddress);
uint32_t ip      = netAddr + hostNum;   // hostNum starts at 2
std::string ipStr = NetworkConfig::uint32ToIp(ip);
allocatedIPs_[subnetName].push_back(ipStr);
nextHost_[subnetName]++;

The helper isInSubnet() uses bitwise masking identical to what hardware routers use:

uint32_t mask = (~0U << (32 - cidr));
return (ipToUint32(ip) & mask) == (ipToUint32(network) & mask);

Allocation example
allocateIP("Lighting")  →  192.168.1.2
allocateIP("Lighting")  →  192.168.1.3
allocateIP("Thermostat")→  192.168.1.66  (64 + 2)
allocateIP("Security")  →  192.168.1.98  (96 + 2)

3.  Routing Table
RoutingTable.cpp implements a software routing table with longest-prefix-match (LPM) lookup. This is the same algorithm used by real routers in BGP and OSPF: when multiple routes match a destination IP, the one with the most specific (longest) prefix wins.

3.1  Route entries
Each entry in the routing table contains:
•	 – the network address (e.g. 192.168.1.64)
•	 – the prefix length (e.g. 27)
•	 – dotted-decimal mask derived from CIDR
•	 – the next-hop IP address
•	 – logical interface name (e.g. “thermostat”, “lan”)
•	 – route preference; lower is preferred

3.2  Longest-prefix-match algorithm
The lookupRoute() function scans all entries and selects the most specific match:

RouteEntry best = defaultRoute_;   // fallback: 0.0.0.0/0
int longestPrefix = -1;
for (const auto& route : routes_) {
  if (isInSubnet(destIP, route.destination, route.cidr)) {
    if (route.cidr > longestPrefix) {
      longestPrefix = route.cidr;
      best = route;
    }
  }
}

Concrete LPM example
Destination: 192.168.1.70
Matches /24  (LAN route, 192.168.1.0/24)       → gateway 192.168.1.1
Matches /27  (Thermostat, 192.168.1.64/27)      → gateway 192.168.1.65
LPM selects /27 because 27 > 24.

Destination: 172.16.5.10  (no match)
Falls back to default route 0.0.0.0/0           → gateway 192.168.1.1

3.3  Routing table for the system
Destination	CIDR	Gateway	Interface	Metric	Purpose
192.168.1.0	/24	192.168.1.1	lan	10	LAN catch-all
192.168.1.0	/26	192.168.1.1	lighting	1	Lighting subnet
192.168.1.64	/27	192.168.1.65	thermostat	1	Thermostat subnet
192.168.1.96	/27	192.168.1.97	security	1	Security subnet
192.168.1.128	/28	192.168.1.129	management	1	Management subnet
0.0.0.0	/0	192.168.1.1	default	0	Default gateway

4.  ARP Table
The Address Resolution Protocol (ARP) maps layer-3 IP addresses to layer-2 MAC addresses so that frames can be delivered on the local network segment. ARPTable.cpp provides a thread-safe simulated ARP cache.

4.1  Entry lifecycle
State	Description
Dynamic	Added by arpRequest() when a MAC is discovered. Expires after 300 seconds (configurable via flushExpired()).
Static	Added manually via addEntry(…, isStatic=true). Never expires, used for gateway entries.
Cache hit	resolve() returns the cached MAC immediately without sending another ARP request.
Cache miss	arpRequest() generates a deterministic MAC from the IP hash and records it as a dynamic entry.

4.2  MAC generation algorithm
Because this is a simulation, MAC addresses are deterministically generated from the IP address using a polynomial hash. This ensures the same IP always produces the same MAC across runs, which makes test assertions reliable:

unsigned int hash = 0;
for (char c : ip) hash = hash * 31 + (unsigned int)c;
// Build XX:XX:XX:XX:XX:XX from hash nibbles
oss << "AA:" << hex(hash>>0) << ":" << hex(hash>>8)
       << ":" << hex(hash>>16) << ":" << hex(hash>>24)
       << ":" << hex(hash>>4);

4.3  Thread safety
All public methods acquire arpMutex_ (a std::mutex) before reading or writing table_. resolve() takes the lock even though it is a const method because std::map iteration must be protected from concurrent writes. The mutex is declared mutable to allow locking in const context.

5.  Socket Communication
The network layer feeds into the TCP socket layer handled by the server. SocketUtils.h provides platform-independent abstractions so the same code compiles on Windows (Winsock2) and Unix/macOS (POSIX sockets).

5.1  Cross-platform abstraction
Item	Windows	Unix / macOS
Socket type	SOCKET	int
Invalid socket	INVALID_SOCKET	-1
Close	closesocket(s)	close(s)
Error code	WSAGetLastError()	strerror(errno)
Init/cleanup	SocketSystem wraps WSAStartup / WSACleanup	Not required

5.2  Server startup sequence
The following steps occur when the server starts on port 5500:
•	 – creates a TCP socket
•	 – allows immediate port reuse after restart
•	 – associates the socket with INADDR_ANY:port
•	 – queues incoming connections
•	 – enters the infinite accept loop

5.3  Client connection flow
Once a client connects, the server:
•	Calls accept() to get a new socket file descriptor for that client
•	Assigns an auto-incremented client_id
•	Creates a ClientHandler object on the heap
•	Spawns std::thread(&ClientHandler::run, handler).detach()
•	Pushes the handler onto client_list (protected by mutex)

6.  Supporting Multiple Devices and Clients
The architecture is specifically designed to allow many clients and many devices to operate concurrently without conflicts. This section explains how the networking layer, threading model, and device state management combine to achieve this.

    6.1  Subnet isolation prevents address conflicts
    Each device category lives in its own subnet. SubnetManager allocates IPs sequentially within that subnet, so two devices of different types can never receive the same IP address. The routing table directs inter-subnet traffic through the correct gateway, meaning a client request intended for a thermostat (192.168.1.64/27) will never be forwarded into the lighting subnet (192.168.1.0/26).

    6.2.  Per-client threads
    Each connected client is given its own dedicated thread (ClientHandler::run). This thread blocks on recv() and handles only that client’s incoming data. Because threads run concurrently, up to MAX_CLIENTS (10) clients can send commands simultaneously without blocking each other.

    Observed behaviour (from terminal output)
    Client 1 connected from 127.0.0.1:60459
    Client 1 is running on thread 0x304b91000
    Client 2 connected from 127.0.0.1:60468
    Client 2 is running on thread 0x304c14000
    
    Each client has a unique thread ID. Client 1 can send while Client 2 sends – both are processed independently.

    6.3  Shared-resource protection
    Three distinct mutex locks protect shared state:
    Mutex	Protects	Why it matters
    TCPServer::mutex	client_list (vector<ClientHandler*>)	broadcast_message() and remove_client() both iterate or modify the list; without a lock two threads could corrupt the vector simultaneously.
    ARPTable::arpMutex_	table_ (map<string, ARPEntry>)	Multiple client threads may trigger ARP lookups for different devices at the same time.
    Device::deviceMutex_	powerOn_, brightness_, temp_, etc.	Client A could turn a light on while Client B reads its status – the mutex ensures the read sees a consistent state.
    
    6.4  Broadcast keeps all clients in sync
    When any client sends a command that changes a device’s state, the server calls broadcast_message() to push the updated state to every other connected client. This ensures that a second client terminal always reflects the real-time state of all devices:
    
    std::string labeled = "Message by [Client " + to_string(sender->get_id())
                        + "]: " + message;
    for (ClientHandler* client : client_list) {
      if (client != sender)
        send(client->get_socket(), labeled.c_str(), labeled.size(), 0);
    }
    
    6.5  Device IP registration through the network layer
    When the server instantiates a device object, it calls SubnetManager::allocateIP() to obtain a valid IP from the correct subnet. It then calls ARPTable::arpRequest() to register an IP-to-MAC mapping for that device. This binds the physical device simulation to the network simulation, so routing lookups and ARP resolution work correctly end-to-end.
    
    Device	Subnet	Example IP	Example MAC
    Light L1	Lighting (192.168.1.0/26)	192.168.1.2	AA:xx:xx:xx:xx:xx
    Light L2	Lighting (192.168.1.0/26)	192.168.1.3	AA:xx:xx:xx:xx:xx
    Thermostat T1	Thermostat (192.168.1.64/27)	192.168.1.66	AA:xx:xx:xx:xx:xx
    SecurityCam C1	Security (192.168.1.96/27)	192.168.1.98	AA:xx:xx:xx:xx:xx

7.  Network Test Results
The network layer was validated with a dedicated test suite (tests/test_network.cpp). All assertions passed on the first build run.

    7.1  ARP table tests
    •	Add an entry and resolve it correctly
    •	Resolve a non-existent IP returns empty string
    •	Remove an entry and confirm it is gone
    •	arpRequest() on a miss generates and caches a MAC
    •	arpRequest() on a hit returns the same cached MAC
    •	flushExpired(0) removes dynamic entries but preserves static entries
    
    7.2  Routing table tests
    •	192.168.1.70 resolves to /27 (thermostat) over /24 (LAN)  —  LPM working
    •	192.168.1.200 resolves to /24 (LAN) since no /26 covers it
    •	10.2.3.4 resolves to /8 route (WAN)
    •	172.16.5.10 falls back to default gateway 0.0.0.0/0
    •	removeRoute() causes 10.2.3.4 to fall back to the default gateway
    
    7.3  SubnetManager tests
    •	initializeSubnets() loads all 4 subnet definitions
    •	First allocateIP("Lighting") returns 192.168.1.2
    •	Second allocateIP("Lighting") returns 192.168.1.3
    •	allocateIP("Thermostat") returns 192.168.1.66
    •	findSubnet() correctly identifies all four subnets from a sample IP
    •	findSubnet() returns "Unknown" for an out-of-range address (10.0.0.10)
    •	allocateIP("UnknownSubnet") throws std::runtime_error

    Test run output
    $ ./test_network
    [INFO] ARP: Who has 192.168.1.20? Tell me.
    [INFO] ARP: 192.168.1.20 is at AA:da:29:69:67:9d
    [INFO] Default gateway set to: 192.168.1.1
    [INFO] Route added: 192.168.1.0/24 via 192.168.1.1
    [INFO] Route added: 192.168.1.64/27 via 192.168.1.65
    [INFO] SubnetManager: Initialized 4 subnets
    [INFO] SubnetManager: Allocated 192.168.1.2 in Lighting
    [INFO] SubnetManager: Allocated 192.168.1.66 in Thermostat
    [INFO] SubnetManager: Released 192.168.1.2 from Lighting
    All network tests passed.

8.  Summary
The network architecture of the Smart Home Automation System achieves the following design goals:

Design goal	How it is met
Efficient IP address utilisation	VLSM divides a single /24 into four right-sized subnets, wasting no address space.
Device category isolation	Each device type (lights, thermostats, cameras) lives in its own subnet; routing directs traffic between them.
Scalable to many devices	Lighting /26 supports up to 62 devices; IP allocation is sequential and tracked by SubnetManager.
Correct inter-subnet routing	Longest-prefix-match routing selects the most specific gateway for any destination IP.
MAC address resolution	ARPTable provides a persistent cache with static/dynamic entries and configurable TTL.
Concurrent multi-client support	Per-client threads and mutex-protected shared state allow up to 10 clients simultaneously.
Platform portability	SocketUtils.h abstracts all platform differences between Windows (Winsock2) and Unix/macOS.

All four networking source files (ARPTable.cpp, RoutingTable.cpp, SubnetManager.cpp, NetworkConfig.cpp) pass the full test suite without warnings or errors.