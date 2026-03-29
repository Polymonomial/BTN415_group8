#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

// Network-wide constants and configuration
namespace NetworkConfig {

// Server settings
constexpr int SERVER_PORT = 8080;
constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int MAX_CLIENTS = 10;
constexpr int BUFFER_SIZE = 4096;

// Base network
constexpr const char* BASE_NETWORK = "192.168.1.0";
constexpr int BASE_CIDR = 24;

// Subnet definitions
struct SubnetInfo {
  std::string name;
  std::string networkAddress;
  std::string subnetMask;
  std::string gateway;
  int cidr;
  int maxHosts;
};

// Get predefined subnet configurations
std::vector<SubnetInfo> getSubnetConfigurations();

// Utility: convert dotted-decimal IP to uint32_t
uint32_t ipToUint32(const std::string& ip);

// Utility: convert uint32_t to dotted-decimal IP
std::string uint32ToIp(uint32_t ip);

// Utility: CIDR to subnet mask string
std::string cidrToMask(int cidr);

// Utility: check if IP is in a given subnet
bool isInSubnet(const std::string& ip, const std::string& network,
                int cidr);

}  // namespace NetworkConfig

#endif  // NETWORK_CONFIG_H