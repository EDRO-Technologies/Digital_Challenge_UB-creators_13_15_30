#include "logger.h"

#include <filesystem>

#include "../utils.h"

void Logger::findNextFileNumber() {
    namespace fs = std::filesystem;

    size_t max_number = 0;
    fs::path base_path(base_filename_);

    // Получаем компоненты пути
    fs::path parent_dir = base_path.parent_path();
    std::string filename = base_path.filename().string();
    std::string basename;
    std::string extension;

    // Разделяем имя файла на базовое имя и расширение
    auto dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        basename = filename.substr(0, dot_pos);
        extension = filename.substr(dot_pos);
    } else {
        basename = filename;
    }

    try {
        // Создаем директорию, если она не существует
        if (!parent_dir.empty() && !fs::exists(parent_dir)) {
            fs::create_directories(parent_dir);
            return;  // Новая директория - начинаем с номера 0
        }

        // Проверяем существование базового файла
        if (fs::exists(base_path)) {
            current_file_size_ = fs::file_size(base_path);
            if (current_file_size_ < max_file_size_) {
                current_file_number_ = 0;
                return;  // Используем существующий базовый файл
            }
        }

        // Перебираем все файлы в директории
        for (const auto& entry :
             fs::directory_iterator(parent_dir.empty() ? "." : parent_dir)) {
            std::string current_filename = entry.path().filename().string();

            // Проверяем, что файл начинается с базового имени
            if (current_filename.find(basename) == 0) {
                // Проверяем файл с номером
                size_t underscore_pos = current_filename.find_last_of('_');
                if (underscore_pos != std::string::npos) {
                    // Извлекаем часть между последним подчеркиванием и
                    // расширением
                    std::string num_str;
                    if (!extension.empty()) {
                        size_t ext_pos =
                            current_filename.find(extension, underscore_pos);
                        if (ext_pos != std::string::npos) {
                            num_str = current_filename.substr(
                                underscore_pos + 1,
                                ext_pos - underscore_pos - 1);
                        }
                    } else {
                        num_str = current_filename.substr(underscore_pos + 1);
                    }

                    // Пробуем преобразовать в число
                    try {
                        size_t num = std::stoul(num_str);
                        max_number = std::max(max_number, num);
                    } catch (...) {
                        // Игнорируем файлы с некорректным форматом номера
                    }
                }
            }
        }
        fs::path max_file_path =
            parent_dir /
            (basename + "_" + std::to_string(max_number) + extension);
        if (fs::exists(max_file_path)) {
            current_file_size_ = fs::file_size(max_file_path);
            if (current_file_size_ < max_file_size_) {
                current_file_number_ = max_number;
                return;  // Используем существующий файл с максимальным номером
            }
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Failed to access log directory: " +
                                 std::string(e.what()));
    }

    // Увеличиваем максимальный номер, так как текущий файл уже заполнен
    current_file_number_ = max_number + 1;
}

std::string Logger::getCurrentFileName() {
    if (current_file_number_ == 0) {
        return base_filename_;
    }

    std::string base_name = base_filename_;
    std::string base_ext;

    auto dot_pos = base_filename_.find_last_of('.');
    if (dot_pos != std::string::npos) {
        base_name = base_filename_.substr(0, dot_pos);
        base_ext = base_filename_.substr(dot_pos);
    }

    return base_name + "_" + std::to_string(current_file_number_) + base_ext;
}

bool Logger::openNewLogFile() {
    if (current_log_file_.is_open()) {
        current_log_file_.close();
    }

    std::string filename = getCurrentFileName();
    current_log_file_.open(filename, std::ios::app | std::ios::binary);

    if (!current_log_file_.is_open()) {
        return false;
    }

    // Определяем текущий размер файла
    current_log_file_.seekp(0, std::ios::end);
    current_file_size_ = current_log_file_.tellp();

    return true;
}

std::string Logger::formatMessage(const std::string& message) {
    std::stringstream ss;
    getCookedLogString(ss) << message;

    return ss.str();
}

void Logger::processQueue() {
    if (!openNewLogFile()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_occurred_ = true;
        }
        error_promise_.set_exception(std::make_exception_ptr(LoggerException(
            "Failed to open initial log file: " + getCurrentFileName())));
        return;
    }

    try {
        error_promise_.set_value();  // Сигнализируем об успешном открытии файла

        while (true) {
            std::string message;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] {
                    return !message_queue_.empty() || !running_;
                });

                if (!running_ && message_queue_.empty()) {
                    break;
                }

                if (!message_queue_.empty()) {
                    message = std::move(message_queue_.front());
                    message_queue_.pop();
                }
            }

            if (!message.empty()) {
                size_t message_full_size =
                    message.length() + 1;  // +1 for new string symbol

                if (!rotateLogFileIfNeeded(message_full_size)) {
                    throw LoggerException("Failed to rotate log file");
                }

                current_log_file_ << message << std::endl;
                if (!current_log_file_) {
                    throw LoggerException("Failed to write to log file");
                }
                current_log_file_.flush();
                current_file_size_ += message_full_size;
            }
        }
    } catch (const std::exception& e) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_occurred_ = true;
        }
        // В случае если promise уже был использован (что означает, что файл был
        // успешно открыт), исключение будет проигнорировано
        try {
            error_promise_.set_exception(std::current_exception());
        } catch (const std::future_error&) {
        }
    }
}