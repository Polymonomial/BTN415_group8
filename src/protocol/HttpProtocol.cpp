#include "protocol/HttpProtocol.h"

#include <sstream>
#include <stdexcept>

#include "devices/Device.h"
#include "devices/Light.h"
#include "devices/SecurityCamera.h"
#include "devices/Thermostat.h"

namespace {

std::vector<std::string> splitPath(const std::string& path) {
  std::vector<std::string> parts;
  std::stringstream ss(path);
  std::string segment;

  while (std::getline(ss, segment, '/')) {
    if (!segment.empty()) {
      parts.push_back(segment);
    }
  }

  return parts;
}

}  // namespace

std::string HttpResponse::toString() const {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << statusCode << " " << statusText << "\n"
      << "Content-Length: " << body.size() << "\n"
      << "\n"
      << body;
  return oss.str();
}

HttpRequest HttpProtocol::parseRequest(
    const std::string& rawRequest) const {
  std::istringstream input(rawRequest);
  std::string method;
  std::string path;

  input >> method >> path;
  if (method.empty() || path.empty()) {
    throw std::runtime_error("Malformed request");
  }

  HttpRequest request{method, path, splitPath(path)};
  return request;
}

HttpResponse HttpProtocol::handleRequest(
    const std::string& rawRequest,
    const std::vector<Device*>& devices) const {
  try {
    return handleRequest(parseRequest(rawRequest), devices);
  } catch (const std::exception& ex) {
    return makeResponse(400, "Bad Request", ex.what());
  }
}

HttpResponse HttpProtocol::handleRequest(
    const HttpRequest& request,
    const std::vector<Device*>& devices) const {
  if (request.method != "GET") {
    return makeResponse(405, "Method Not Allowed",
                        "Only GET is supported");
  }

  if (request.segments.size() >= 2 && request.segments[0] == "light") {
    return handleLightRequest(request, devices);
  }

  if (request.segments.size() >= 2 &&
      request.segments[0] == "thermostat") {
    return handleThermostatRequest(request, devices);
  }

  if (request.segments.size() >= 2 &&
      request.segments[0] == "security") {
    return handleSecurityRequest(request, devices);
  }

  if (request.segments.size() == 2 &&
      request.segments[0] == "devices" &&
      request.segments[1] == "list") {
    return handleDeviceListRequest(devices);
  }

  return makeResponse(404, "Not Found", "Unknown command path");
}

HttpResponse HttpProtocol::makeResponse(
    int statusCode,
    const std::string& statusText,
    const std::string& body) const {
  return HttpResponse{statusCode, statusText, body};
}

Device* HttpProtocol::findFirstDeviceOfType(
    Device::DeviceType type,
    const std::vector<Device*>& devices) const {
  for (Device* device : devices) {
    if (device != nullptr && device->getType() == type) {
      return device;
    }
  }
  return nullptr;
}

HttpResponse HttpProtocol::handleLightRequest(
    const HttpRequest& request,
    const std::vector<Device*>& devices) const {
  Device* device =
      findFirstDeviceOfType(Device::DeviceType::LIGHT, devices);
  if (device == nullptr) {
    return makeResponse(404, "Not Found", "No light device registered");
  }

  auto* light = dynamic_cast<Light*>(device);
  if (light == nullptr) {
    return makeResponse(500, "Internal Server Error",
                        "Registered light has invalid type");
  }

  if (request.segments[1] == "status") {
    return makeResponse(200, "OK", light->getDetailedStatus());
  }

  if (request.segments[1] == "on") {
    light->turnOn();
    return makeResponse(200, "OK", "Light turned on");
  }

  if (request.segments[1] == "off") {
    light->turnOff();
    return makeResponse(200, "OK", "Light turned off");
  }

  if (request.segments[1] == "brightness" &&
      request.segments.size() == 3) {
    int level = std::stoi(request.segments[2]);
    light->setBrightness(level);
    return makeResponse(200, "OK",
                        "Light brightness set to " +
                            std::to_string(light->getBrightness()));
  }

  return makeResponse(404, "Not Found", "Unknown light command");
}

HttpResponse HttpProtocol::handleThermostatRequest(
    const HttpRequest& request,
    const std::vector<Device*>& devices) const {
  Device* device = findFirstDeviceOfType(
      Device::DeviceType::THERMOSTAT, devices);
  if (device == nullptr) {
    return makeResponse(404, "Not Found",
                        "No thermostat device registered");
  }

  auto* thermostat = dynamic_cast<Thermostat*>(device);
  if (thermostat == nullptr) {
    return makeResponse(500, "Internal Server Error",
                        "Registered thermostat has invalid type");
  }

  if (request.segments[1] == "status") {
    return makeResponse(200, "OK", thermostat->getDetailedStatus());
  }

  if (request.segments[1] == "set" && request.segments.size() == 3) {
    double target = std::stod(request.segments[2]);
    thermostat->turnOn();
    thermostat->setTargetTemperature(target);
    return makeResponse(
        200, "OK",
        "Thermostat target set to " +
            std::to_string(thermostat->getTargetTemperature()));
  }

  return makeResponse(404, "Not Found",
                      "Unknown thermostat command");
}

HttpResponse HttpProtocol::handleSecurityRequest(
    const HttpRequest& request,
    const std::vector<Device*>& devices) const {
  Device* device = findFirstDeviceOfType(
      Device::DeviceType::SECURITY_CAMERA, devices);
  if (device == nullptr) {
    return makeResponse(404, "Not Found",
                        "No security camera registered");
  }

  auto* camera = dynamic_cast<SecurityCamera*>(device);
  if (camera == nullptr) {
    return makeResponse(500, "Internal Server Error",
                        "Registered security device has invalid type");
  }

  if (request.segments[1] == "status") {
    return makeResponse(200, "OK", camera->getDetailedStatus());
  }

  if (request.segments[1] == "arm") {
    camera->arm();
    return makeResponse(200, "OK", "Security camera armed");
  }

  if (request.segments[1] == "disarm") {
    camera->disarm();
    return makeResponse(200, "OK", "Security camera disarmed");
  }

  return makeResponse(404, "Not Found", "Unknown security command");
}

HttpResponse HttpProtocol::handleDeviceListRequest(
    const std::vector<Device*>& devices) const {
  std::ostringstream oss;

  if (devices.empty()) {
    oss << "No devices registered";
    return makeResponse(200, "OK", oss.str());
  }

  for (Device* device : devices) {
    if (device == nullptr) {
      continue;
    }

    oss << device->getName() << " [" << device->getId() << "] - "
        << device->getTypeString() << " - " << device->getIpAddress()
        << " - " << device->getStatus() << "\n";
  }

  std::string body = oss.str();
  if (!body.empty() && body.back() == '\n') {
    body.pop_back();
  }

  return makeResponse(200, "OK", body);
}
