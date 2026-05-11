#include "core/AccessLogger.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace cw_db {

    using json = nlohmann::json;

    AccessLogger::AccessLogger(const std::string& file_path) {
        // ios::app гарантирует, что мы дописываем в конец файла, а не перезаписываем его
        log_file.open(file_path, std::ios::app);
        if (!log_file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл access.log для записи");
        }
    }

    AccessLogger::~AccessLogger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    void AccessLogger::log_request(const std::string& client_id,
                                   const std::string& handler_id,
                                   const std::string& query_body,
                                   uint64_t start_time_ms,
                                   uint64_t end_time_ms,
                                   int status_code) {
        
        // Упаковываем данные в JSON
        json log_entry;
        log_entry["client_id"] = client_id;
        log_entry["handler_id"] = handler_id;
        log_entry["query"] = query_body;
        log_entry["start_time"] = start_time_ms;
        log_entry["end_time"] = end_time_ms;
        log_entry["duration_ms"] = end_time_ms - start_time_ms;
        log_entry["status"] = status_code;

        // БЛОКИРУЕМ ПОТОК: С этой строки только один поток может работать с файлом
        std::lock_guard<std::mutex> lock(mtx);

        // Пишем строку и сразу сбрасываем буфер на диск (flush)
        log_file << log_entry.dump() << "\n";
        log_file.flush(); 

        // При выходе из функции lock_guard автоматически снимет блокировку
    }

} // namespace cw_db