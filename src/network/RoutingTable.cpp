/*
Routing: check whether the routing table chooses the correct next hop or route for a destination IP. Review RoutingTable.cpp and test with sample destination networks.
*/

#include "network/RoutingTable.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "network/NetworkConfig.h"
#include "utils/Logger.h"

RoutingTable::RoutingTable() {
  // Initialize default route
  defaultRoute_ = {"0.0.0.0", 0, "0.0.0.0", "0.0.0.0",
                    "default", 0};
}

void RoutingTable::addRoute(const std::string& destination,
                            int cidr, const std::string& gateway,
                            const std::string& interface_,
                            int metric) {
                              std::lock_guard<std::mutex> lock(tableMutex_);
                              RouteEntry entry;
                              entry.destination = destination;
                              entry.cidr = cidr;
                              entry.subnetMask = NetworkConfig::cidrToMask(cidr);
                              entry.gateway = gateway;
                              entry.interface_ = interface_;
                              entry.metric = metric;
                              routes_.push_back(entry);
                            
                              Logger::getInstance().info("Route added: " + destination + "/" +
                                                         std::to_string(cidr) + " via " +
                                                         gateway);
                            }

void RoutingTable::removeRoute(const std::string& destination,
                               int cidr) {
  std::lock_guard<std::mutex> lock(tableMutex_);
  routes_.erase(
      std::remove_if(routes_.begin(), routes_.end(),
                     [&](const RouteEntry& e) {
                       return e.destination == destination &&
                              e.cidr == cidr;
                     }),
      routes_.end());
}

RouteEntry RoutingTable::lookupRoute(
    const std::string& destIP) const {
  std::lock_guard<std::mutex> lock(tableMutex_);

  // Longest prefix match
  RouteEntry bestMatch = defaultRoute_;
  int longestPrefix = -1;

  for (const auto& route : routes_) {
    if (NetworkConfig::isInSubnet(destIP, route.destination,
                                  route.cidr)) {
      if (route.cidr > longestPrefix) {
        longestPrefix = route.cidr;
        bestMatch = route;
      }
    }
  }
  return bestMatch;
}

void RoutingTable::setDefaultGateway(
    const std::string& gateway, const std::string& interface_) {
  std::lock_guard<std::mutex> lock(tableMutex_);
  defaultRoute_.gateway = gateway;
  defaultRoute_.interface_ = interface_;
  Logger::getInstance().info("Default gateway set to: " + gateway);
}

void RoutingTable::printTable() const {
  std::lock_guard<std::mutex> lock(tableMutex_);
  std::cout << "\n=== Routing Table ===" << std::endl;
  std::cout << "Destination\t\tMask\t\t\tGateway\t\t\tInterface\t"
               "Metric"
            << std::endl;
  std::cout << std::string(90, '-') << std::endl;

  for (const auto& r : routes_) {
    std::cout << r.destination << "/" << r.cidr << "\t\t"
              << r.subnetMask << "\t\t" << r.gateway << "\t\t"
              << r.interface_ << "\t\t" << r.metric << std::endl;
  }

  std::cout << "0.0.0.0/0\t\t0.0.0.0\t\t\t"
            << defaultRoute_.gateway << "\t\t"
            << defaultRoute_.interface_ << "\t\t0 (default)"
            << std::endl;
}