#include "protocol/ICMPHandler.h"

#include <sstream>

#include "devices/Device.h"
#include "network/ARPTable.h"
#include "network/NetworkConfig.h"
#include "utils/Logger.h"

ICMPResult ICMPHandler::ping(const std::string& ipAddress,
                             ARPTable* arpTable) const {
  if (ipAddress.empty()) {
    return {false, "", "", "PING failed: empty IP address"};
  }

  std::string macAddress;
  if (arpTable != nullptr) {
    macAddress = arpTable->arpRequest(ipAddress);
  }

  const bool reachable =
      NetworkConfig::ipToUint32(ipAddress) != 0U || ipAddress == "0.0.0.0";

  ICMPResult result{
      reachable,
      ipAddress,
      macAddress,
      reachable ? "ICMP echo reply received from " + ipAddress
                : "ICMP destination unreachable for " + ipAddress};

  Logger::getInstance().info(result.message);
  return result;
}

ICMPResult ICMPHandler::ping(const Device& device,
                             ARPTable* arpTable) const {
  return ping(device.getIpAddress(), arpTable);
}

std::string ICMPHandler::formatResult(
    const ICMPResult& result) const {
  std::ostringstream oss;
  oss << "PING " << result.ipAddress << "\n"
      << "Status: " << (result.reachable ? "reachable" : "unreachable")
      << "\n";

  if (!result.macAddress.empty()) {
    oss << "Resolved MAC: " << result.macAddress << "\n";
  }

  oss << "Message: " << result.message;
  return oss.str();
}
