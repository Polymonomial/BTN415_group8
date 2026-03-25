#ifndef LOGGER_H
#define LOGGER_H

#include <mutex>
#include <string>

// Thread-safe logger
class Logger {
 public:
  enum class Level { DEBUG, INFO, WARNING, ERROR };

  // Singleton access
  static Logger& getInstance();

  void setLevel(Level level);
  void debug(const std::string& message);
  void info(const std::string& message);
  void warning(const std::string& message);
  void error(const std::string& message);

 private:
  Logger();
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void log(Level level, const std::string& message);
  std::string levelToString(Level level) const;
  std::string getCurrentTimestamp() const;

  Level currentLevel_;
  std::mutex logMutex_;
};

#endif  // LOGGER_H