#pragma once

#include "storage/FileManager.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace cw_db {

    // Типы операций для журнала
    enum class OpType : uint8_t { INSERT, UPDATE, DELETE };

    // Жестко упакованная структура одной записи в логе (25 байт)
    #pragma pack(push, 1)
    struct LogEntry {
        uint64_t timestamp;  // Время операции в миллисекундах (UNIX Time)
        OpType type;         // Что сделали
        uint64_t pk;         // С каким ключом
        uint64_t old_offset; // Какое смещение было ДО этой операции
    };
    #pragma pack(pop)

    class TransactionLog {
    private:
        std::unique_ptr<FileManager> file_manager;
        uint64_t current_eof;

    public:
        explicit TransactionLog(std::unique_ptr<FileManager> fm);

        // Записать операцию в конец лога
        void append_log(uint64_t timestamp, OpType type, uint64_t pk, uint64_t old_offset);

        // Прочитать историю задом наперед (с конца до нужного времени)
        std::vector<LogEntry> read_logs_backwards_until(uint64_t target_timestamp);
    };

} // namespace cw_db