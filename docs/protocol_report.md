# Protocol Design Report
## BTN415 – Data Communication Programming
### Smart Home Network Automation System — Group 8

---

## 1. Overview

The protocol layer defines how client applications communicate with smart home devices through the server. Rather than sending raw strings over the TCP socket, the system uses a structured **HTTP-like request/response protocol** that gives every command a consistent format, predictable status codes, and a serializable wire representation.

Two classes make up this layer:

| Class | File | Responsibility |
|----------------|---------------------------------|--------------------------------------------------------|
| `HttpProtocol` | `src/protocol/HttpProtocol.cpp` | Parse requests, dispatch to devices, build responses   |
| `ICMPHandler`  | `src/protocol/ICMPHandler.cpp`  | Simulate ICMP ping/echo with ARP-backed MAC resolution |

The protocol layer sits between the TCP server (which delivers raw bytes) and the device objects (which hold physical state). It is stateless — every `handleRequest()` call is independent.

---

## 2. Request Format

### 2.1 Structure

Every request is a single line in the format:

```
METHOD /segment1/segment2[/param]
```

The only supported method is `GET`. The path is split on `/` into a `segments` vector, which the dispatcher uses to route the request.

**Examples:**

```
GET /light/on
GET /light/brightness/75
GET /thermostat/set/24
GET /thermostat/status
GET /security/arm
GET /security/disarm
GET /devices/list
```

### 2.2 Parsing — `parseRequest()`

```cpp
HttpRequest HttpProtocol::parseRequest(const std::string& rawRequest) const {
  std::istringstream input(rawRequest);
  std::string method, path;
  input >> method >> path;

  if (method.empty() || path.empty()) {
    throw std::runtime_error("Malformed request");
  }

  return HttpRequest{method, path, splitPath(path)};
}
```

`splitPath()` tokenises the path on `/`, skipping empty segments so a leading slash does not produce a blank first entry. The resulting `segments` vector drives all dispatch logic downstream.

**Malformed input** (no space, missing path, empty string) throws `std::runtime_error`, which the string overload of `handleRequest()` catches and converts to a `400 Bad Request` response — so the server never crashes on bad client input.

---

## 3. Response Format

### 3.1 Structure

```cpp
struct HttpResponse {
  int         statusCode;
  std::string statusText;
  std::string body;
};
```

### 3.2 Wire serialisation — `toString()`

```cpp
std::string HttpResponse::toString() const {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << statusCode << " " << statusText << "\n"
      << "Content-Length: " << body.size() << "\n"
      << "\n"
      << body;
  return oss.str();
}
```

**Example wire output:**

```
HTTP/1.1 200 OK
Content-Length: 15

Light turned on
```

The `Content-Length` header lets the receiver know exactly how many bytes to read for the body, which prevents partial-read bugs when the body contains newlines.

---

## 4. Request Dispatch

### 4.1 Method check

All requests that are not `GET` are immediately rejected:

```cpp
if (request.method != "GET") {
  return makeResponse(405, "Method Not Allowed", "Only GET is supported");
}
```

### 4.2 Path routing

The first path segment determines the device category:

```
segments[0] == "light"       → handleLightRequest()
segments[0] == "thermostat"  → handleThermostatRequest()
segments[0] == "security"    → handleSecurityRequest()
segments[0] == "devices"
  && segments[1] == "list"   → handleDeviceListRequest()
anything else                → 404 Not Found
```

### 4.3 Device lookup — `findFirstDeviceOfType()`

Each handler calls this helper to locate the first device of the required type in the shared device vector:

```cpp
Device* HttpProtocol::findFirstDeviceOfType(
    Device::DeviceType type,
    const std::vector<Device*>& devices) const {
  for (Device* device : devices) {
    if (device != nullptr && device->getType() == type)
      return device;
  }
  return nullptr;
}
```

If no matching device is found, the handler returns `404 Not Found`. If the `dynamic_cast` to the concrete type fails (type mismatch in the device list), it returns `500 Internal Server Error`.

---

## 5. Command Reference

### 5.1 Light commands

| Request                   | Action                                  | Success response                         |
|---------------------------|-----------------------------------------|------------------------------------------|
| `GET /light/on`           | `turnOn()`                              | `200 Light turned on`                    |
| `GET /light/off`          | `turnOff()`                             | `200 Light turned off`                   |
| `GET /light/brightness/N` | `setBrightness(N)` — clamps to [0, 100] | `200 Light brightness set to N`          |
| `GET /light/status`       | `getDetailedStatus()`                   | `200` + full status block                |

### 5.2 Thermostat commands

| Request                 | Action                                 | Success response                 |
|-------------------------|----------------------------------------|----------------------------------|
| `GET /thermostat/set/N` | `turnOn()` + `setTargetTemperature(N)` | `200 Thermostat target set to N` |
| `GET /thermostat/status`| `getDetailedStatus()`                  | `200` + full status block.       |

`setTargetTemperature()` clamps input to [10.0, 35.0] °C inside the `Thermostat` class, so the protocol layer passes the raw value through and lets the device enforce its own bounds.

### 5.3 Security camera commands

| Request                | Action.                               | Success response               |
|------------------------|---------------------------------------|--------------------------------|
| `GET /security/arm`    | `arm()` — also sets `powerOn_ = true` | `200 Security camera armed`    |
| `GET /security/disarm` | `disarm()` — clears motion flag       | `200 Security camera disarmed` |
| `GET /security/status` | `getDetailedStatus()`                 | `200` + full status block      |

### 5.4 Device listing

`GET /devices/list` iterates the full device vector and returns one line per device:

```
Living Room Light [L1] - Light - 192.168.1.2 - OFF
Main Thermostat [T1] - Thermostat - 192.168.1.66 - OFF
Front Door Camera [C1] - SecurityCamera - 192.168.1.98 - OFF
```

---

## 6. Error Handling

| Condition                       | Status code              | Message                               |
|---------------------------------| -------------------------|---------------------------------------|
| Method is not GET               | `405 Method Not Allowed` | `Only GET is supported`               |
| Unknown first segment or path   | `404 Not Found`          | `Unknown command path`                |
| No device of the requested type | `404 Not Found`          | e.g. `No light device registered`     |
| `dynamic_cast` returns null     | `500 Internal Server Error`| `Registered light has invalid type` |
| Malformed request (no space / empty) | `400 Bad Request`   | Exception message from `parseRequest()`|
| Unknown sub-command for a device | `404 Not Found`         | e.g. `Unknown light command`          |

The string overload wraps the parsed overload in a `try/catch`:

```cpp
HttpResponse HttpProtocol::handleRequest(
    const std::string& rawRequest,
    const std::vector<Device*>& devices) const {
  try {
    return handleRequest(parseRequest(rawRequest), devices);
  } catch (const std::exception& ex) {
    return makeResponse(400, "Bad Request", ex.what());
  }
}
```

This means **no exception ever propagates out of the protocol layer** — the server thread always gets a valid `HttpResponse` back, regardless of what the client sent.

---

## 7. ICMP Handler

### 7.1 Purpose

`ICMPHandler` simulates the ICMP ping/echo mechanism used in real networks to test reachability. It works alongside `ARPTable` — before reporting reachability it resolves the target IP to a MAC address, exactly as a real ping implementation would trigger an ARP lookup before sending the ICMP echo request.

### 7.2 `ICMPResult` struct

```cpp
struct ICMPResult {
  bool        reachable;
  std::string ipAddress;
  std::string macAddress;
  std::string message;
};
```

### 7.3 `ping()` — IP overload

```cpp
ICMPResult ICMPHandler::ping(const std::string& ipAddress,
                             ARPTable* arpTable) const {
  if (ipAddress.empty()) {
    return {false, "", "", "PING failed: empty IP address"};
  }

  std::string macAddress;
  if (arpTable != nullptr) {
    macAddress = arpTable->arpRequest(ipAddress);   // triggers ARP if not cached
  }

  const bool reachable = NetworkConfig::ipToUint32(ipAddress) != 0U
                         || ipAddress == "0.0.0.0";

  return {reachable, ipAddress, macAddress,
          reachable ? "ICMP echo reply received from " + ipAddress
                    : "ICMP destination unreachable for " + ipAddress};
}
```

The reachability check uses `ipToUint32()` — any valid non-zero IP is considered reachable in the simulation. An empty string short-circuits immediately and returns unreachable without touching the ARP table.

### 7.4 `ping()` — Device overload

```cpp
ICMPResult ICMPHandler::ping(const Device& device,
                             ARPTable* arpTable) const {
  return ping(device.getIpAddress(), arpTable);
}
```

This convenience overload extracts the IP from any `Device` object and delegates to the IP overload, so callers do not need to inspect device internals directly.

### 7.5 `formatResult()`

Produces a human-readable multi-line summary:

```
PING 192.168.1.66
Status: reachable
Resolved MAC: AA:5c:2a:69:67:a5
Message: ICMP echo reply received from 192.168.1.66
```

### 7.6 Integration with the network layer

`ICMPHandler` depends directly on `ARPTable` (your networking code). When `ping()` calls `arpTable->arpRequest()`, it triggers the full ARP lookup flow — cache check, MAC generation on a miss, and cache population — the same path exercised by the network tests. This connects the protocol layer to the networking layer end-to-end.

---

## 8. Protocol Test Results

Test file: `tests/test_protocol.cpp`

### 8.1 Test functions

| Test function            | What it verifies                                     |
|--------------------------|------------------------------------------------------|
| `test_request_parsing()` | `parseRequest()` extracts method, path, and segments correctly; malformed input throws |
| `test_light_commands()`  | on/off/brightness/status commands update device state and return correct responses |
| `test_thermostat_commands()` | set/status commands update temperature and power state |
| `test_security_commands()` | arm/disarm/status commands toggle armed flag correctly |
| `test_device_listing()`    | `/devices/list` returns all three devices; `toString()` starts with `HTTP/1.1 200 OK` and includes `Content-Length` |
| `test_http_errors()`       | POST → 405, unknown path → 404, malformed string → 400 |
| `test_icmp()`              | ping by IP triggers ARP, ping by device uses device IP, empty IP returns unreachable, `formatResult()` output is correct |

### 8.2 Build command

```bash
g++ -Wall -std=c++17 -Iinclude \
  -o test_protocol \
  tests/test_protocol.cpp \
  src/protocol/HttpProtocol.cpp \
  src/protocol/ICMPHandler.cpp \
  src/devices/Device.cpp \
  src/devices/Light.cpp \
  src/devices/Thermostat.cpp \
  src/devices/SecurityCamera.cpp \
  src/network/ARPTable.cpp \
  src/network/NetworkConfig.cpp \
  src/utils/Logger.cpp
```

### 8.3 Terminal output

```
[2026-03-30 02:42:28] [INFO] ARP: Who has 192.168.1.20? Tell me.
[2026-03-30 02:42:28] [INFO] ARP: 192.168.1.20 is at AA:da:29:69:67:9d
[2026-03-30 02:42:28] [INFO] ICMP echo reply received from 192.168.1.20
[2026-03-30 02:42:28] [INFO] ARP: Who has 192.168.1.66? Tell me.
[2026-03-30 02:42:28] [INFO] ARP: 192.168.1.66 is at AA:5c:2a:69:67:a5
[2026-03-30 02:42:28] [INFO] ICMP echo reply received from 192.168.1.66
All protocol tests passed.
```

The ARP log lines confirm that `ICMPHandler` correctly triggers `arpRequest()` for each IP it pings, and the MAC addresses produced are consistent with the deterministic hash algorithm in `ARPTable`.

---

## 9. Summary

| Design goal.                | How it is met |
|-----------------------------| ---------------------------------------------------------------------------|
| Structured command language | HTTP-like `METHOD /path/segments` format with a typed `HttpRequest` struct |
| Consistent error responses  | Every error condition returns a specific status code (400/404/405/500) — no unhandled exceptions |
| Stateless protocol layer    | `HttpProtocol` holds no member state; all methods are `const` |
| Device-agnostic dispatch    | `findFirstDeviceOfType()` searches the device list by enum, decoupled from concrete class names |
| Reachability simulation.    | `ICMPHandler::ping()` triggers ARP resolution before reporting reachability, mirroring real ICMP behaviour |
| Full test coverage | Seven test functions covering parsing, all three device types, listing, all error codes, and ICMP — all assertions pass |
