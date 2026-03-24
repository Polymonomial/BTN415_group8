# Smart Home Network Automation System

BTN 415 - Data Communications Programming Project

## Overview
A multithreaded client-server smart home automation system built in C++.
Users control simulated devices (lights, thermostat, security cameras) over
a TCP network using an HTTP-like application protocol. The system implements
subnetting, VLSM, static routing, ARP resolution, and ICMP.

## Network Design
- Base network: 192.168.1.0/24
- Subnet 1 (Lighting):    192.168.1.0/26   — up to 62 hosts
- Subnet 2 (Thermostat):  192.168.1.64/27  — up to 30 hosts
- Subnet 3 (Security):    192.168.1.96/27  — up to 30 hosts
- Subnet 4 (Management):  192.168.1.128/28 — up to 14 hosts

## Build Instructions

### Prerequisites
- C++17 compiler (g++, clang++, or MSVC)
- CMake 3.16+

### Build
mkdir build && cd build
cmake ..
cmake --build .

### Run
# Terminal 1 – start server
./server

# Terminal 2 – start client
./client

### Run Tests
cd build
ctest --verbose

## Protocol Commands
GET /light/status
GET /light/on
GET /light/off
GET /light/brightness/<0-100>
GET /thermostat/status
GET /thermostat/set/<temperature>
GET /security/status
GET /security/arm
GET /security/disarm
GET /devices/list
PING <ip_address>
ARP <ip_address>

## Project Structure
include/    — header files
src/        — source files
tests/      — unit & integration tests
docs/       — report template

## Team Members
- Member 1: Brendan Chang 105403182
- Member 2: Idriss Lufungula
- Member 3: Naoko Marin