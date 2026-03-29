#include "network/ARPTable.h"
#include "network/RoutingTable.h"
#include "network/SubnetManager.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

void test_arp_table() {
  ARPTable arp;

  arp.addEntry("192.168.1.10", "AA:BB:CC:DD:EE:10");
  assert(arp.resolve("192.168.1.10") == "AA:BB:CC:DD:EE:10");
  assert(arp.resolve("192.168.1.250").empty());

  arp.removeEntry("192.168.1.10");
  assert(arp.resolve("192.168.1.10").empty());

  const std::string learnedMac = arp.arpRequest("192.168.1.20");
  assert(!learnedMac.empty());
  assert(arp.resolve("192.168.1.20") == learnedMac);
  assert(arp.arpRequest("192.168.1.20") == learnedMac);

  arp.addEntry("192.168.1.30", "AA:BB:CC:DD:EE:30", true);
  arp.addEntry("192.168.1.31", "AA:BB:CC:DD:EE:31", false);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  arp.flushExpired(0);

  assert(arp.resolve("192.168.1.30") == "AA:BB:CC:DD:EE:30");
  assert(arp.resolve("192.168.1.31").empty());
}

void test_routing_table() {
  RoutingTable routing;

  routing.setDefaultGateway("192.168.1.1", "default");
  routing.addRoute("192.168.1.0", 24, "192.168.1.1", "lan", 10);
  routing.addRoute("192.168.1.64", 27, "192.168.1.65", "thermostat", 1);
  routing.addRoute("10.0.0.0", 8, "10.0.0.1", "wan", 5);

  RouteEntry bestLocal = routing.lookupRoute("192.168.1.70");
  assert(bestLocal.destination == "192.168.1.64");
  assert(bestLocal.cidr == 27);
  assert(bestLocal.gateway == "192.168.1.65");
  assert(bestLocal.interface_ == "thermostat");

  RouteEntry broadLocal = routing.lookupRoute("192.168.1.200");
  assert(broadLocal.destination == "192.168.1.0");
  assert(broadLocal.cidr == 24);
  assert(broadLocal.interface_ == "lan");

  RouteEntry remote = routing.lookupRoute("10.2.3.4");
  assert(remote.destination == "10.0.0.0");
  assert(remote.cidr == 8);
  assert(remote.gateway == "10.0.0.1");

  RouteEntry fallback = routing.lookupRoute("172.16.5.10");
  assert(fallback.destination == "0.0.0.0");
  assert(fallback.cidr == 0);
  assert(fallback.gateway == "192.168.1.1");
  assert(fallback.interface_ == "default");

  routing.removeRoute("10.0.0.0", 8);
  RouteEntry removedFallback = routing.lookupRoute("10.2.3.4");
  assert(removedFallback.destination == "0.0.0.0");
  assert(removedFallback.gateway == "192.168.1.1");
}

void test_subnet_manager() {
  SubnetManager manager;
  manager.initializeSubnets();

  NetworkConfig::SubnetInfo lighting = manager.getSubnetInfo("Lighting");
  assert(lighting.networkAddress == "192.168.1.0");
  assert(lighting.subnetMask == "255.255.255.192");
  assert(lighting.gateway == "192.168.1.1");
  assert(lighting.cidr == 26);

  const std::string firstLightingIp = manager.allocateIP("Lighting");
  const std::string secondLightingIp = manager.allocateIP("Lighting");
  const std::string thermostatIp = manager.allocateIP("Thermostat");

  assert(firstLightingIp == "192.168.1.2");
  assert(secondLightingIp == "192.168.1.3");
  assert(thermostatIp == "192.168.1.66");

  assert(manager.findSubnet(firstLightingIp) == "Lighting");
  assert(manager.findSubnet("192.168.1.70") == "Thermostat");
  assert(manager.findSubnet("192.168.1.100") == "Security");
  assert(manager.findSubnet("192.168.1.130") == "Management");
  assert(manager.findSubnet("10.0.0.10") == "Unknown");

  manager.releaseIP("Lighting", firstLightingIp);

  bool threwForMissingSubnet = false;
  try {
    manager.allocateIP("UnknownSubnet");
  } catch (const std::runtime_error&) {
    threwForMissingSubnet = true;
  }
  assert(threwForMissingSubnet);
}

}  // namespace

int main() {
  test_arp_table();
  test_routing_table();
  test_subnet_manager();

  std::cout << "All network tests passed." << std::endl;
  return 0;
}
