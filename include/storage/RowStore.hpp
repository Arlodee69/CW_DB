#pragma once

#include "storage/FileManager.hpp"
#include "core/Row.hpp"
#include <memory>

namespace cw_db {

    class RowStore {
    private:
        std::unique_ptr<FileManager> file_manager;
        
        // Указатель на конец файла (куда дописывать новые строки)
        uint64_t current_eof;

    public:
        explicit RowStore(std::unique_ptr<FileManager> fm);

        // Записать строку в файл и получить её смещение (которое пойдет в дерево)
        uint64_t append_row(const Row& row);

        // Прочитать строку из файла по смещению
        // Нам нужно знать num_columns, чтобы понять, сколько байт читать
        Row get_row(uint64_t offset, size_t num_columns);
    };

} // namespace cw_db