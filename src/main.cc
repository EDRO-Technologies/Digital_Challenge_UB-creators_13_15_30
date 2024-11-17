#include <iostream>
#include <string>

#include "logger/logger.h"
#include "server/server.h"
#include "utils.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: DNSServer <config_file>" << std::endl;
        return 1;
    }

    ServerConfiguration server_config;
    const std::string config_filename = std::string(argv[1]);

    try {
        parseServerConfiguration(server_config, config_filename);
    } catch (const ConfigurateException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    Logger* logger = nullptr;
    DNSServer* server = nullptr;
    boost::asio::io_context io_context;

    try {
        logger =
            new Logger(server_config.base_filename, server_config.max_log_size);
        auto future = logger->start();

        // Ждём успешной инициализации или ошибки
        future.get();

        std::stringstream ss;
        getCookedLogString(ss) << "Logger started: " << std::endl;
        std::cout << ss.str();

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

        server = new DNSServer(server_config.port, io_context,
                               server_config.base_dns_ip, *logger);

        signals.async_wait([&](const boost::system::error_code& ec,
                               int signal_number) {
            if (!ec) {
                std::stringstream ss;
                getCookedLogString(ss) << "Received signal " << signal_number
                                       << ". Stopping server..." << std::endl;
                std::cout << ss.str();

                // Останавливаем сервер
                server->stop();

                // Останавливаем io_context
                io_context.stop();

                // Останавливаем логгер
                logger->stop();
            }
        });

        server->start();

        ss.clear();
        getCookedLogString(ss) << "Server started." << std::endl;
        std::cout << ss.str();

        io_context.run();

        std::stringstream final_ss;
        getCookedLogString(final_ss)
            << "Application shutdown complete." << std::endl;
        std::cout << final_ss.str();
    } catch (const std::exception& e) {
        // Останавливаем сервер
        if (server) server->stop();

        // Останавливаем io_context
        io_context.stop();

        // Останавливаем логгер
        if (logger) logger->stop();

        std::stringstream ss;
        getCookedLogString(ss) << "Error: " << e.what() << std::endl;
        std::cerr << ss.str();
        return 1;
    }
    return 0;
}

void parseServerConfiguration(ServerConfiguration& p_conf,
                              const std::string& conf_filename) {
    try {
        // Загружаем YAML-файл
        YAML::Node config = YAML::LoadFile(conf_filename);

        // Парсим значения из файла
        if (config["log_filename"]) {
            p_conf.base_filename = config["log_filename"].as<std::string>();
        }
        if (config["logfile_size"]) {
            p_conf.max_log_size = config["logfile_size"].as<size_t>();
        }
        if (config["port"]) {
            p_conf.port = config["port"].as<uint16_t>();
        }
        if (config["dns_server"]) {
            p_conf.base_dns_ip = config["dns_server"].as<std::string>();
        }
    } catch (const YAML::Exception& e) {
        throw ConfigurateException("Error parsing YAML configuration: " +
                                   std::string(e.what()));
    } catch (const std::exception& e) {
        throw ConfigurateException("Error loading configuration file: " +
                                   std::string(e.what()));
    }
}