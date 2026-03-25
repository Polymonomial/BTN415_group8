#include "utils/Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger& Logger::getInstance() {
  static Logger instance;
  return instance;
}

Logger::Logger() : currentLevel_(Level::INFO) {}

void Logger::setLevel(Level level) { currentLevel_ = level; }

void Logger::debug(const std::string& message) {
  log(Level::DEBUG, message);
}

void Logger::info(const std::string& message) {
  log(Level::INFO, message);
}

void Logger::warning(const std::string& message) {
  log(Level::WARNING, message);
}

void Logger::error(const std::string& message) {
  log(Level::ERROR, message);
}

void Logger::log(Level level, const std::string& message) {
  if (level < currentLevel_) {
    return;
  }

  std::lock_guard<std::mutex> lock(logMutex_);
  std::cout << "[" << getCurrentTimestamp() << "] "
            << "[" << levelToString(level) << "] " << message
            << std::endl;
}

std::string Logger::levelToString(Level level) const {
  switch (level) {
    case Level::DEBUG:
      return "DEBUG";
    case Level::INFO:
      return "INFO";
    case Level::WARNING:
      return "WARN";
    case Level::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

std::string Logger::getCurrentTimestamp() const {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  return oss.str();
}