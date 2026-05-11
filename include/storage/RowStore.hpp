#pragma once

#include "storage/FileManager.hpp"
#include <vector>
#include <cstdint>
#include <memory>

namespace cw_db {

    // Структура строки: просто массив 8-байтовых ячеек
    struct Row {
        std::vector<uint64_t> cells;
    };

    class RowStore {
    private:
        std::unique_ptr<FileManager> file_manager;
        uint64_t current_eof;

    public:
        explicit RowStore(std::unique_ptr<FileManager> fm);
        
        // Добавить строку в конец файла и вернуть ее смещение (offset)
        uint64_t append_row(const Row& row);
        
        // Прочитать строку по смещению
        Row read_row(uint64_t offset, size_t num_columns);
    };

} // namespace cw_db