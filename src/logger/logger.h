#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <future>
#include <queue>

class LoggerException : public std::exception {
   public:
    explicit LoggerException(const std::string& message) : message_(message) {}
    explicit LoggerException(const char* message) : message_(message) {}

    virtual const char* what() const noexcept override {
        return message_.c_str();
    }

   private:
    std::string message_;
};

class Logger {
   public:
    explicit Logger(const std::string& base_filename, size_t max_file_size)
        : running_(false),
          error_occurred_(false),
          base_filename_(base_filename),
          max_file_size_(max_file_size * 1024),
          current_file_size_(0) {
        findNextFileNumber();
    }

    ~Logger() { stop(); }

    // Начало работы логгера с возвратом future для отслеживания ошибок
    std::future<void> start() {
        running_ = true;
        error_promise_ = std::promise<void>();
        auto future = error_promise_.get_future();

        worker_thread_ = std::thread(&Logger::processQueue, this);
        return future;
    }

    // Остановка работы логгера
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        condition_.notify_one();

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    // Добавление сообщения в очередь
    void log(const std::string& message) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (error_occurred_) {
                throw LoggerException("Logger is in error state");
            }
            message_queue_.push(formatMessage(message));
        }
        condition_.notify_one();
    }

    void operator<<(const std::string& message) { log(message); }

    bool hasError() const { return error_occurred_; }

   private:
    std::queue<std::string> message_queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_thread_;
    bool running_;
    bool error_occurred_;
    std::promise<void> error_promise_;

    std::string base_filename_;
    size_t max_file_size_;
    size_t current_file_number_{0};
    size_t current_file_size_;
    std::ofstream current_log_file_;

    void findNextFileNumber();

    std::string getCurrentFileName();

    bool openNewLogFile();

    bool rotateLogFileIfNeeded(size_t message_size) {
        if (current_file_size_ + message_size > max_file_size_) {
            current_file_number_++;
            return openNewLogFile();
        }
        return true;
    }

    // Форматирование сообщения с добавлением временной метки
    std::string formatMessage(const std::string& message);

    // Основной цикл обработки очереди сообщений
    void processQueue();
};

#endif  // LOGGER_H