#ifndef HTTP_PROTOCOL_H
#define HTTP_PROTOCOL_H

#include <string>
#include <vector>

#include "devices/Device.h"

struct HttpRequest {
  std::string method;
  std::string path;
  std::vector<std::string> segments;
};

struct HttpResponse {
  int statusCode;
  std::string statusText;
  std::string body;

  std::string toString() const;
};

class HttpProtocol {
 public:
  HttpRequest parseRequest(const std::string& rawRequest) const;
  HttpResponse handleRequest(const std::string& rawRequest,
                             const std::vector<Device*>& devices) const;
  HttpResponse handleRequest(const HttpRequest& request,
                             const std::vector<Device*>& devices) const;

 private:
  HttpResponse makeResponse(int statusCode,
                            const std::string& statusText,
                            const std::string& body) const;
  Device* findFirstDeviceOfType(
      Device::DeviceType type,
      const std::vector<Device*>& devices) const;
  HttpResponse handleLightRequest(
      const HttpRequest& request,
      const std::vector<Device*>& devices) const;
  HttpResponse handleThermostatRequest(
      const HttpRequest& request,
      const std::vector<Device*>& devices) const;
  HttpResponse handleSecurityRequest(
      const HttpRequest& request,
      const std::vector<Device*>& devices) const;
  HttpResponse handleDeviceListRequest(
      const std::vector<Device*>& devices) const;
};

#endif  // HTTP_PROTOCOL_H
