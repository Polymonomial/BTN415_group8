#ifndef ICMP_HANDLER_H
#define ICMP_HANDLER_H

#include <string>

class ARPTable;
class Device;

struct ICMPResult {
  bool reachable;
  std::string ipAddress;
  std::string macAddress;
  std::string message;
};

class ICMPHandler {
 public:
  ICMPResult ping(const std::string& ipAddress, ARPTable* arpTable) const;
  ICMPResult ping(const Device& device, ARPTable* arpTable) const;
  std::string formatResult(const ICMPResult& result) const;
};

#endif  // ICMP_HANDLER_H
