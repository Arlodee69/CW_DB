#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <cstdint>

namespace cw_db {

    class AccessLogger {
    private:
        std::ofstream log_file;
        std::mutex mtx; // Тот самый мьютекс для защиты от одновременной записи

    public:
        // Открываем файл в режиме добавления (append)
        explicit AccessLogger(const std::string& file_path);
        ~AccessLogger();

        // Метод, который будет вызывать напарник после каждого запроса
        void log_request(const std::string& client_id,
                         const std::string& handler_id,
                         const std::string& query_body,
                         uint64_t start_time_ms,
                         uint64_t end_time_ms,
                         int status_code);
    };

} // namespace cw_db