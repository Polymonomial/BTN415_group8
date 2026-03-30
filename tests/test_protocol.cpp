#include "devices/Light.h"
#include "devices/SecurityCamera.h"
#include "devices/Thermostat.h"
#include "network/ARPTable.h"
#include "protocol/HttpProtocol.h"
#include "protocol/ICMPHandler.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

// Convert owned device instances into the raw pointer view expected by
// the protocol handlers.
std::vector<Device*> makeDeviceView(
    const std::vector<std::unique_ptr<Device>>& devices) {
  std::vector<Device*> view;
  for (const auto& device : devices) {
    view.push_back(device.get());
  }
  return view;
}

// Validate request-line parsing and malformed-request handling.
void test_request_parsing() {
  HttpProtocol protocol;

  HttpRequest request = protocol.parseRequest("GET /light/status");
  assert(request.method == "GET");
  assert(request.path == "/light/status");
  assert(request.segments.size() == 2);
  assert(request.segments[0] == "light");
  assert(request.segments[1] == "status");

  bool threw = false;
  try {
    protocol.parseRequest("GET_ONLY");
  } catch (const std::runtime_error&) {
    threw = true;
  }
  assert(threw);
}

// Exercise the light-related HTTP-like commands end to end.
void test_light_commands(const std::vector<Device*>& devices) {
  HttpProtocol protocol;

  HttpResponse onResponse =
      protocol.handleRequest("GET /light/on", devices);
  assert(onResponse.statusCode == 200);
  assert(onResponse.body == "Light turned on");

  auto* light = dynamic_cast<Light*>(devices[0]);
  assert(light != nullptr);
  assert(light->isOn());

  HttpResponse brightnessResponse =
      protocol.handleRequest("GET /light/brightness/75", devices);
  assert(brightnessResponse.statusCode == 200);
  assert(light->getBrightness() == 75);

  HttpResponse statusResponse =
      protocol.handleRequest("GET /light/status", devices);
  assert(statusResponse.statusCode == 200);
  assert(statusResponse.body.find("Brightness: 75%") !=
         std::string::npos);

  HttpResponse offResponse =
      protocol.handleRequest("GET /light/off", devices);
  assert(offResponse.statusCode == 200);
  assert(!light->isOn());
}

// Confirm thermostat command dispatch updates device state correctly.
void test_thermostat_commands(const std::vector<Device*>& devices) {
  HttpProtocol protocol;
  auto* thermostat = dynamic_cast<Thermostat*>(devices[1]);
  assert(thermostat != nullptr);

  HttpResponse setResponse =
      protocol.handleRequest("GET /thermostat/set/24", devices);
  assert(setResponse.statusCode == 200);
  assert(thermostat->isOn());
  assert(thermostat->getTargetTemperature() == 24.0);

  HttpResponse statusResponse =
      protocol.handleRequest("GET /thermostat/status", devices);
  assert(statusResponse.statusCode == 200);
  assert(statusResponse.body.find("Target Temp: 24") !=
         std::string::npos);
}

// Verify arm/disarm/status commands for the security camera path.
void test_security_commands(const std::vector<Device*>& devices) {
  HttpProtocol protocol;
  auto* camera = dynamic_cast<SecurityCamera*>(devices[2]);
  assert(camera != nullptr);

  HttpResponse armResponse =
      protocol.handleRequest("GET /security/arm", devices);
  assert(armResponse.statusCode == 200);
  assert(camera->isArmed());

  HttpResponse statusResponse =
      protocol.handleRequest("GET /security/status", devices);
  assert(statusResponse.statusCode == 200);
  assert(statusResponse.body.find("Armed: YES") != std::string::npos);

  HttpResponse disarmResponse =
      protocol.handleRequest("GET /security/disarm", devices);
  assert(disarmResponse.statusCode == 200);
  assert(!camera->isArmed());
}

// Ensure the protocol can serialize a simple inventory response.
void test_device_listing(const std::vector<Device*>& devices) {
  HttpProtocol protocol;

  HttpResponse response =
      protocol.handleRequest("GET /devices/list", devices);
  assert(response.statusCode == 200);
  assert(response.body.find("Living Room Light [L1]") !=
         std::string::npos);
  assert(response.body.find("Main Thermostat [T1]") !=
         std::string::npos);
  assert(response.body.find("Front Door Camera [C1]") !=
         std::string::npos);

  std::string wireFormat = response.toString();
  assert(wireFormat.find("HTTP/1.1 200 OK") == 0);
  assert(wireFormat.find("Content-Length: ") != std::string::npos);
}

// Check protocol-level error responses for unsupported or malformed input.
void test_http_errors(const std::vector<Device*>& devices) {
  HttpProtocol protocol;

  HttpResponse badMethod =
      protocol.handleRequest("POST /light/on", devices);
  assert(badMethod.statusCode == 405);

  HttpResponse unknownPath =
      protocol.handleRequest("GET /unknown/path", devices);
  assert(unknownPath.statusCode == 404);

  HttpResponse malformed =
      protocol.handleRequest("BROKEN_REQUEST", devices);
  assert(malformed.statusCode == 400);
}

// Validate simulated ICMP reachability plus ARP-backed MAC resolution.
void test_icmp(const std::vector<Device*>& devices) {
  ARPTable arpTable;
  ICMPHandler handler;

  ICMPResult directPing =
      handler.ping("192.168.1.20", &arpTable);
  assert(directPing.reachable);
  assert(directPing.ipAddress == "192.168.1.20");
  assert(!directPing.macAddress.empty());

  auto* thermostat = dynamic_cast<Thermostat*>(devices[1]);
  assert(thermostat != nullptr);

  ICMPResult devicePing = handler.ping(*thermostat, &arpTable);
  assert(devicePing.reachable);
  assert(devicePing.ipAddress == thermostat->getIpAddress());

  ICMPResult emptyPing = handler.ping("", &arpTable);
  assert(!emptyPing.reachable);
  assert(emptyPing.message == "PING failed: empty IP address");

  std::string formatted = handler.formatResult(devicePing);
  assert(formatted.find("PING " + thermostat->getIpAddress()) !=
         std::string::npos);
  assert(formatted.find("Status: reachable") != std::string::npos);
  assert(formatted.find("Resolved MAC: ") != std::string::npos);
}

}  // namespace

int main() {
  // Build a representative in-memory device set for protocol testing.
  std::vector<std::unique_ptr<Device>> ownedDevices;
  ownedDevices.push_back(std::make_unique<Light>(
      "L1", "Living Room Light", "192.168.1.2",
      "AA:BB:CC:DD:EE:01", "Lighting"));
  ownedDevices.push_back(std::make_unique<Thermostat>(
      "T1", "Main Thermostat", "192.168.1.66",
      "AA:BB:CC:DD:EE:02", "Thermostat"));
  ownedDevices.push_back(std::make_unique<SecurityCamera>(
      "C1", "Front Door Camera", "192.168.1.98",
      "AA:BB:CC:DD:EE:03", "Security"));

  std::vector<Device*> devices = makeDeviceView(ownedDevices);

  test_request_parsing();
  test_light_commands(devices);
  test_thermostat_commands(devices);
  test_security_commands(devices);
  test_device_listing(devices);
  test_http_errors(devices);
  test_icmp(devices);

  std::cout << "All protocol tests passed." << std::endl;
  return 0;
}
