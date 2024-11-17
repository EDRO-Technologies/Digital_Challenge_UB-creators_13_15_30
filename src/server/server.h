#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <cstdint>

#include "../logger/logger.h"

using boost::asio::ip::udp;

constexpr size_t MAX_DNS_PACKET_SIZE = 512;  // Максимальный размер DNS пакета

class DNSServer {
   public:
    DNSServer(const uint16_t dns_port, boost::asio::io_context& io_context,
              const std::string& forward_address, Logger& logger)
        : socket_(io_context, udp::endpoint(udp::v4(), dns_port)),
          forward_socket_(io_context),
          resolver_(io_context),
          forward_address_(forward_address),
          logger_(logger) {
        // Резолвим адрес форвард-сервера
        auto endpoints = resolver_.resolve(udp::v4(), forward_address_, "53");
        forward_endpoint_ = *endpoints.begin();

        // Открываем сокет для пересылки
        forward_socket_.open(udp::v4());
    }

    void start() { receive(); }

    void stop() {
        // Отменяем все асинхронные операции
        boost::system::error_code ec;
        socket_.cancel(ec);

        // Закрываем сокет
        socket_.close(ec);
    }

   private:
    struct QueryContext {
        std::vector<uint8_t> buffer;
        udp::endpoint client_endpoint;
        uint16_t query_id;

        QueryContext(const uint8_t* data, size_t size,
                     const udp::endpoint& endpoint)
            : buffer(data, data + size), client_endpoint(endpoint) {
            // Извлекаем ID запроса из DNS-заголовка (первые 2 байта)
            if (size >= 2) {
                query_id = (static_cast<uint16_t>(data[0]) << 8) |
                           static_cast<uint16_t>(data[1]);
            }
        }
    };

    udp::socket socket_;
    udp::socket forward_socket_;
    udp::resolver resolver_;
    udp::endpoint forward_endpoint_;
    udp::endpoint sender_endpoint_;
    std::string forward_address_;
    std::array<uint8_t, MAX_DNS_PACKET_SIZE> data_;
    Logger& logger_;

    void receive() {
        data_.fill(0);

        socket_.async_receive_from(
            boost::asio::buffer(data_, MAX_DNS_PACKET_SIZE), sender_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    handleRequest(bytes_recvd);
                }
                receive();
            });
    }

    void handleRequest(std::size_t bytes_recvd);

    void startReceiveResponse(std::shared_ptr<QueryContext> context);
    void handleResponse(
        std::shared_ptr<QueryContext> context,
        std::shared_ptr<std::array<uint8_t, MAX_DNS_PACKET_SIZE>>
            response_buffer,
        std::size_t bytes_transferred);

    class DNSNameExtractor {
       private:
        // Константы для DNS пакета
        static constexpr uint8_t DNS_COMPRESSION_MASK = 0xC0;
        static constexpr uint8_t DNS_COMPRESSION_FLAG = 0xC0;
        static constexpr size_t MAX_DNS_LENGTH = 255;
        static constexpr size_t MAX_LABEL_LENGTH = 63;

       public:
        static std::string extractDomainName(
            const std::array<uint8_t, MAX_DNS_PACKET_SIZE> buffer,
            size_t buffer_size, size_t offset = 12);
    };
};

#endif  // SERVER_H