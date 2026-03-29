// SubnetManager.h
/**
Subnetting: check whether subnet calculations are correct, such as network address, subnet mask, host range, or whether two IPs belong to the same subnet. That logic should be in SubnetManager.cpp.
*/

#ifndef SUBNET_MANAGER_H
#define SUBNET_MANAGER_H

#include <map>
#include <string>
#include <vector>

#include "network/NetworkConfig.h"

// Manages VLSM subnets and IP address allocation
class SubnetManager {
 public:
  SubnetManager();
  ~SubnetManager() = default;

  // Initialize subnets from configuration
  void initializeSubnets();

  // Allocate the next available IP in a subnet
  std::string allocateIP(const std::string& subnetName);

  // Release an IP back to the pool
  void releaseIP(const std::string& subnetName,
                 const std::string& ip);

  // Get subnet info by name
  NetworkConfig::SubnetInfo getSubnetInfo(
      const std::string& subnetName) const;

  // Determine which subnet an IP belongs to
  std::string findSubnet(const std::string& ip) const;

  // Print all subnet allocations (for debugging / report)
  void printSubnetAllocations() const;

 private:
  // subnet name -> subnet info
  std::map<std::string, NetworkConfig::SubnetInfo> subnets_;
  // subnet name -> list of allocated IPs
  std::map<std::string, std::vector<std::string>> allocatedIPs_;
  // subnet name -> next host number to allocate
  std::map<std::string, int> nextHost_;
};

#endif  // SUBNET_MANAGER_H