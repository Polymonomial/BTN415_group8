#include "network/NetworkConfig.h"

#include <arpa/inet.h>

#include <cstdint>
#include <sstream>
#include <vector>

// On Windows, replace <arpa/inet.h> usage with Winsock equivalents
// The CMake build handles linking ws2_32

namespace NetworkConfig {

std::vector<SubnetInfo> getSubnetConfigurations() {
  // VLSM design from 192.168.1.0/24
  return {
      {"Lighting", "192.168.1.0", "255.255.255.192",
       "192.168.1.1", 26, 62},
      {"Thermostat", "192.168.1.64", "255.255.255.224",
       "192.168.1.65", 27, 30},
      {"Security", "192.168.1.96", "255.255.255.224",
       "192.168.1.97", 27, 30},
      {"Management", "192.168.1.128", "255.255.255.240",
       "192.168.1.129", 28, 14},
  };
}

uint32_t ipToUint32(const std::string& ip) {
  uint32_t result = 0;
  std::istringstream stream(ip);
  std::string octet;
  int shift = 24;
  while (std::getline(stream, octet, '.')) {
    result |= (static_cast<uint32_t>(std::stoi(octet)) << shift);
    shift -= 8;
  }
  return result;
}

std::string uint32ToIp(uint32_t ip) {
  return std::to_string((ip >> 24) & 0xFF) + "." +
         std::to_string((ip >> 16) & 0xFF) + "." +
         std::to_string((ip >> 8) & 0xFF) + "." +
         std::to_string(ip & 0xFF);
}

std::string cidrToMask(int cidr) {
  uint32_t mask = (cidr == 0) ? 0 : (~0U << (32 - cidr));
  return uint32ToIp(mask);
}

bool isInSubnet(const std::string& ip, const std::string& network,
                int cidr) {
  uint32_t ipAddr = ipToUint32(ip);
  uint32_t netAddr = ipToUint32(network);
  uint32_t mask = (cidr == 0) ? 0 : (~0U << (32 - cidr));
  return (ipAddr & mask) == (netAddr & mask);
}

}  // namespace NetworkConfig