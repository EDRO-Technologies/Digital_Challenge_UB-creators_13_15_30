#include "server.h"

#include <cstdint>
#include <iostream>

#include "../utils.h"

void DNSServer::handleRequest(std::size_t bytes_recvd) {
    auto context = std::make_shared<QueryContext>(data_.data(), bytes_recvd,
                                                  sender_endpoint_);

    try {
        std::string domain_name =
            DNSNameExtractor::extractDomainName(data_, bytes_recvd);
        std::string log_message =
            sender_endpoint_.address().to_string() + " " + domain_name;

        logger_ << log_message;
    } catch (const LoggerException& e) {
        std::stringstream ss;
        getCookedLogString(ss) << "Error: " << e.what() << std::endl;

        std::cerr << ss.str();  // На будущее - написать систему логгирования
    }

    forward_socket_.async_send_to(
        boost::asio::buffer(context->buffer), forward_endpoint_,
        [this, context](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                startReceiveResponse(context);
            }
        });
}

void DNSServer::startReceiveResponse(std::shared_ptr<QueryContext> context) {
    auto response_buffer =
        std::make_shared<std::array<uint8_t, MAX_DNS_PACKET_SIZE>>();

    forward_socket_.async_receive_from(
        boost::asio::buffer(*response_buffer), forward_endpoint_,
        [this, context, response_buffer](boost::system::error_code ec,
                                         std::size_t bytes_transferred) {
            if (!ec) {
                handleResponse(context, response_buffer, bytes_transferred);
            }
        });
}

void DNSServer::handleResponse(
    std::shared_ptr<QueryContext> context,
    std::shared_ptr<std::array<uint8_t, MAX_DNS_PACKET_SIZE>> response_buffer,
    std::size_t bytes_transferred) {
    // Проверяем ID ответа
    if (bytes_transferred >= 2) {
        uint16_t response_id =
            (static_cast<uint16_t>((*response_buffer)[0]) << 8) |
            static_cast<uint16_t>((*response_buffer)[1]);

        if (response_id == context->query_id) {
            // Отправляем ответ клиенту через основной сокет
            socket_.async_send_to(
                boost::asio::buffer(*response_buffer, bytes_transferred),
                context->client_endpoint,
                [](boost::system::error_code ec, std::size_t) {
                    if (ec) {
                        std::cerr << "Error sending response to client: "
                                  << ec.message() << std::endl;
                    }
                });
        }
    }
}

std::string DNSServer::DNSNameExtractor::extractDomainName(
    const std::array<uint8_t, MAX_DNS_PACKET_SIZE> buffer, size_t buffer_size,
    size_t offset) {
    if (buffer_size < offset) {
        throw std::runtime_error("Buffer too small");
    }

    // Предварительно резервируем память для среднего размера домена
    std::string result;
    result.reserve(64);

    size_t pos = offset;
    uint8_t jumps = 0;
    const uint8_t max_jumps = 10;  // Защита от циклических ссылок

    while (pos < buffer_size) {
        // Получаем длину текущей метки
        uint8_t label_length = buffer[pos];

        // Проверяем на конец имени
        if (label_length == 0) {
            break;
        }

        // Проверяем на сжатое имя
        if ((label_length & DNS_COMPRESSION_MASK) == DNS_COMPRESSION_FLAG) {
            if (pos + 1 >= buffer_size) {
                throw std::runtime_error("Invalid compression pointer");
            }

            if (++jumps > max_jumps) {
                throw std::runtime_error("Too many compression pointers");
            }

            // Вычисляем смещение для сжатого имени
            size_t new_pos =
                ((label_length & ~DNS_COMPRESSION_MASK) << 8) | buffer[pos + 1];
            if (new_pos >= pos) {
                throw std::runtime_error("Invalid forward compression pointer");
            }
            pos = new_pos;
            continue;
        }

        // Проверяем корректность длины метки
        if (label_length > MAX_LABEL_LENGTH) {
            throw std::runtime_error("Label too long");
        }

        if (pos + 1 + label_length > buffer_size) {
            throw std::runtime_error("Label exceeds buffer");
        }

        // Добавляем разделитель, если это не первая метка
        if (!result.empty()) {
            result.push_back('.');
        }

        // Добавляем символы метки
        result.append(reinterpret_cast<const char*>(&buffer[pos + 1]),
                      label_length);

        // Проверяем общую длину имени
        if (result.length() > MAX_DNS_LENGTH) {
            throw std::runtime_error("Domain name too long");
        }

        pos += label_length + 1;
    }

    return result;
}