/*
ARP: check whether the ARP table correctly maps IP addresses to MAC addresses. Look at ARPTable.cpp and confirm you can add entries, retrieve them, and handle missing entries.
*/

#include "network/ARPTable.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "utils/Logger.h"

ARPTable::ARPTable() {}

std::string ARPTable::resolve(const std::string& ipAddress) const {
  std::lock_guard<std::mutex> lock(arpMutex_);
  auto it = table_.find(ipAddress);
  if (it != table_.end()) {
    return it->second.macAddress;
  }
  return "";
}

void ARPTable::addEntry(const std::string& ipAddress,
                        const std::string& macAddress,
                        bool isStatic) {
  std::lock_guard<std::mutex> lock(arpMutex_);
  ARPEntry entry;
  entry.macAddress = macAddress;
  entry.timestamp = std::chrono::steady_clock::now();
  entry.isStatic = isStatic;
  table_[ipAddress] = entry;
  Logger::getInstance().debug("ARP entry added: " + ipAddress +
                              " -> " + macAddress);
}

void ARPTable::removeEntry(const std::string& ipAddress) {
  std::lock_guard<std::mutex> lock(arpMutex_);
  table_.erase(ipAddress);
}

std::string ARPTable::arpRequest(const std::string& ipAddress) {
  // Check cache first
  std::string cached = resolve(ipAddress);
  if (!cached.empty()) {
    Logger::getInstance().debug("ARP cache hit for " + ipAddress);
    return cached;
  }

  // Simulate ARP request/reply
  Logger::getInstance().info("ARP: Who has " + ipAddress +
                             "? Tell me.");
  std::string mac = generateMAC(ipAddress);
  Logger::getInstance().info("ARP: " + ipAddress + " is at " + mac);

  addEntry(ipAddress, mac, false);
  return mac;
}

void ARPTable::flushExpired(int maxAgeSecs) {
  std::lock_guard<std::mutex> lock(arpMutex_);
  auto now = std::chrono::steady_clock::now();

  for (auto it = table_.begin(); it != table_.end();) {
    if (!it->second.isStatic) {
      auto age = std::chrono::duration_cast<std::chrono::seconds>(
                     now - it->second.timestamp)
                     .count();
      if (age > maxAgeSecs) {
        Logger::getInstance().debug("ARP: Flushing expired entry " +
                                    it->first);
        it = table_.erase(it);
        continue;
      }
    }
    ++it;
  }
}

void ARPTable::printTable() const {
  std::lock_guard<std::mutex> lock(arpMutex_);
  std::cout << "\n=== ARP Table ===" << std::endl;
  std::cout << "IP Address\t\tMAC Address\t\tType" << std::endl;
  std::cout << std::string(60, '-') << std::endl;
  for (const auto& [ip, entry] : table_) {
    std::cout << ip << "\t\t" << entry.macAddress << "\t"
              << (entry.isStatic ? "Static" : "Dynamic")
              << std::endl;
  }
}

std::string ARPTable::generateMAC(const std::string& ip) const {
  // Deterministic MAC generation from IP for simulation
  unsigned int hash = 0;
  for (char c : ip) {
    hash = hash * 31 + static_cast<unsigned int>(c);
  }

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << "AA:" << std::setw(2) << ((hash >> 0) & 0xFF) << ":"
      << std::setw(2) << ((hash >> 8) & 0xFF) << ":"
      << std::setw(2) << ((hash >> 16) & 0xFF) << ":"
      << std::setw(2) << ((hash >> 24) & 0xFF) << ":"
      << std::setw(2) << ((hash >> 4) & 0xFF);
  return oss.str();
}