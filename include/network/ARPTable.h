/*
ARP: check whether the ARP table correctly maps IP addresses to MAC addresses. Look at ARPTable.cpp and confirm you can add entries, retrieve them, and handle missing entries.
*/

#ifndef ARP_TABLE_H
#define ARP_TABLE_H

#include <chrono>
#include <map>
#include <mutex>
#include <string>

// ARP cache entry
struct ARPEntry {
  std::string macAddress;
  std::chrono::steady_clock::time_point timestamp;
  bool isStatic;
};

// Simulated ARP table for IP-to-MAC resolution
class ARPTable {
 public:
  ARPTable();
  ~ARPTable() = default;

  // Resolve an IP address to a MAC address
  // Returns empty string if not found
  std::string resolve(const std::string& ipAddress) const;

  // Add / update an ARP entry
  void addEntry(const std::string& ipAddress,
                const std::string& macAddress,
                bool isStatic = false);

  // Remove an entry
  void removeEntry(const std::string& ipAddress);

  // Simulate ARP request/reply
  std::string arpRequest(const std::string& ipAddress);

  // Flush expired dynamic entries
  void flushExpired(int maxAgeSecs = 300);

  // Print the ARP table
  void printTable() const;

 private:
  mutable std::mutex arpMutex_;
  std::map<std::string, ARPEntry> table_;

  // Generate a simulated MAC address for an IP
  std::string generateMAC(const std::string& ip) const;
};

#endif  // ARP_TABLE_H