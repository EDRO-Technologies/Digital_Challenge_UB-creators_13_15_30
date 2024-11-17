#ifndef UTILS_H
#define UTILS_H

#include <yaml-cpp/yaml.h>

#include <chrono>
#include <iomanip>

class ConfigurateException : public std::exception {
   public:
    explicit ConfigurateException(const std::string& message)
        : message_(message) {}
    explicit ConfigurateException(const char* message) : message_(message) {}

    virtual const char* what() const noexcept override {
        return message_.c_str();
    }

   private:
    std::string message_;
};

struct ServerConfiguration {
    std::string base_filename;
    size_t max_log_size;
    uint16_t port;
    std::string base_dns_ip;

    ServerConfiguration()
        : base_filename(""), max_log_size(0), port(0), base_dns_ip("") {}
    ServerConfiguration(const std::string& base_filename, size_t max_log_size,
                        uint16_t port, const std::string& base_dns_ip)
        : base_filename(base_filename),
          max_log_size(max_log_size),
          port(port),
          base_dns_ip(base_dns_ip) {}
};

void parseServerConfiguration(ServerConfiguration& p_conf,
                              const std::string& conf_filename);

inline std::stringstream& getCookedLogString(std::stringstream& ss) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.'
       << std::setfill('0') << std::setw(3) << ms.count() << ": ";

    return ss;
}

#endif