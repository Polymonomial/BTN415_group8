#include "network/SubnetManager.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "utils/Logger.h"

SubnetManager::SubnetManager() {}

void SubnetManager::initializeSubnets() {
  auto configs = NetworkConfig::getSubnetConfigurations();
  for (const auto& cfg : configs) {
    subnets_[cfg.name] = cfg;
    allocatedIPs_[cfg.name] = {};
    // Start allocating from host number 2
    // (1 is reserved for gateway)
    nextHost_[cfg.name] = 2;
  }
  Logger::getInstance().info(
      "SubnetManager: Initialized " +
      std::to_string(configs.size()) + " subnets");
}

std::string SubnetManager::allocateIP(
    const std::string& subnetName) {
  auto it = subnets_.find(subnetName);
  if (it == subnets_.end()) {
    throw std::runtime_error("Subnet not found: " + subnetName);
  }

  const auto& subnet = it->second;
  int hostNum = nextHost_[subnetName];

  if (hostNum > subnet.maxHosts) {
    throw std::runtime_error("No more IPs available in subnet: " +
                             subnetName);
  }

  uint32_t netAddr = NetworkConfig::ipToUint32(subnet.networkAddress);
  uint32_t ip = netAddr + hostNum;
  std::string ipStr = NetworkConfig::uint32ToIp(ip);

  allocatedIPs_[subnetName].push_back(ipStr);
  nextHost_[subnetName]++;

  Logger::getInstance().info("SubnetManager: Allocated " + ipStr +
                             " in " + subnetName);
  return ipStr;
}

void SubnetManager::releaseIP(const std::string& subnetName,
                              const std::string& ip) {
  auto it = allocatedIPs_.find(subnetName);
  if (it != allocatedIPs_.end()) {
    auto& ips = it->second;
    ips.erase(std::remove(ips.begin(), ips.end(), ip), ips.end());
    Logger::getInstance().info("SubnetManager: Released " + ip +
                               " from " + subnetName);
  }
}

NetworkConfig::SubnetInfo SubnetManager::getSubnetInfo(
    const std::string& subnetName) const {
  auto it = subnets_.find(subnetName);
  if (it == subnets_.end()) {
    throw std::runtime_error("Subnet not found: " + subnetName);
  }
  return it->second;
}

std::string SubnetManager::findSubnet(const std::string& ip) const {
  for (const auto& [name, info] : subnets_) {
    if (NetworkConfig::isInSubnet(ip, info.networkAddress,
                                  info.cidr)) {
      return name;
    }
  }
  return "Unknown";
}

void SubnetManager::printSubnetAllocations() const {
  std::cout << "\n=== Subnet Allocations ===" << std::endl;
  for (const auto& [name, info] : subnets_) {
    std::cout << "Subnet: " << name << " ("
              << info.networkAddress << "/" << info.cidr << ")"
              << std::endl;
    std::cout << "  Mask: " << info.subnetMask << std::endl;
    std::cout << "  Gateway: " << info.gateway << std::endl;
    std::cout << "  Max Hosts: " << info.maxHosts << std::endl;

    auto allocIt = allocatedIPs_.find(name);
    if (allocIt != allocatedIPs_.end()) {
      std::cout << "  Allocated IPs:" << std::endl;
      for (const auto& ip : allocIt->second) {
        std::cout << "    - " << ip << std::endl;
      }
    }
    std::cout << std::endl;
  }
}