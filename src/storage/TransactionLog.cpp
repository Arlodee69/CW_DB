#include "storage/TransactionLog.hpp"
#include <cstring>
#include <algorithm>

namespace cw_db {

    TransactionLog::TransactionLog(std::unique_ptr<FileManager> fm) 
        : file_manager(std::move(fm)) 
    {
        current_eof = file_manager->get_size();
    }

    void TransactionLog::append_log(uint64_t timestamp, OpType type, uint64_t pk, uint64_t old_offset) {
        size_t entry_size = sizeof(LogEntry);

        if (current_eof + entry_size > file_manager->get_size()) {
            file_manager->grow_file(file_manager->get_size() + 4096);
        }

        LogEntry entry = { timestamp, type, pk, old_offset };
        
        char* write_ptr = static_cast<char*>(file_manager->get_data()) + current_eof;
        std::memcpy(write_ptr, &entry, entry_size);

        current_eof += entry_size;
    }

    std::vector<LogEntry> TransactionLog::read_logs_backwards_until(uint64_t target_timestamp) {
        std::vector<LogEntry> result;
        if (current_eof == 0) return result;

        int64_t offset = current_eof - sizeof(LogEntry);
        char* raw_data = static_cast<char*>(file_manager->get_data());

        // Идем с конца файла в начало (LIFO - Last In, First Out)
        while (offset >= 0) {
            LogEntry entry;
            std::memcpy(&entry, raw_data + offset, sizeof(LogEntry));

            // Если мы дошли до записей, которые были сделаны ДО целевого времени — останавливаемся
            if (entry.timestamp <= target_timestamp) {
                break;
            }

            result.push_back(entry);
            offset -= sizeof(LogEntry);
        }

        return result;
    }

} // namespace cw_db