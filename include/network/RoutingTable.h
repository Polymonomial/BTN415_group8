/** Routing: check whether the routing table chooses the correct next hop or route for a destination IP. Review RoutingTable.cpp and test with sample destination networks. */

#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include <mutex>
#include <string>
#include <vector>

// Represents a single route entry
struct RouteEntry {
  std::string destination;  // network address
  int cidr;                 // prefix length
  std::string subnetMask;
  std::string gateway;      // next-hop gateway
  std::string interface_;   // outgoing interface / subnet name
  int metric;               // cost
};

// Static routing table for inter-subnet communication
class RoutingTable {
 public:
  RoutingTable();
  ~RoutingTable() = default;

  // Add a route
  void addRoute(const std::string& destination, int cidr,
                const std::string& gateway,
                const std::string& interface_, int metric = 1);

  // Remove a route
  void removeRoute(const std::string& destination, int cidr);

  // Look up the best route for a destination IP
  RouteEntry lookupRoute(const std::string& destIP) const;

  // Set the default gateway
  void setDefaultGateway(const std::string& gateway,
                         const std::string& interface_);

  // Print the routing table
  void printTable() const;

 private:
  mutable std::mutex tableMutex_;
  std::vector<RouteEntry> routes_;
  RouteEntry defaultRoute_;
};

#endif  // ROUTING_TABLE_H