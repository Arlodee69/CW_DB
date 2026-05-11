#include "storage/RowStore.hpp"
#include <cstring>

namespace cw_db {

    RowStore::RowStore(std::unique_ptr<FileManager> fm) : file_manager(std::move(fm)) {
        current_eof = file_manager->get_size();
    }

    uint64_t RowStore::append_row(const Row& row) {
        size_t row_size = row.cells.size() * sizeof(uint64_t);
        
        // Если файл заполнился, увеличиваем его размер
        if (current_eof + row_size > file_manager->get_size()) {
            file_manager->grow_file(file_manager->get_size() + 4096);
        }

        // Копируем данные из вектора прямо в память файла
        char* write_ptr = static_cast<char*>(file_manager->get_data()) + current_eof;
        std::memcpy(write_ptr, row.cells.data(), row_size);

        uint64_t written_offset = current_eof;
        current_eof += row_size;

        return written_offset;
    }

    Row RowStore::read_row(uint64_t offset, size_t num_columns) {
        Row row;
        row.cells.resize(num_columns);
        
        // Читаем сырые байты из mmap обратно в вектор
        char* read_ptr = static_cast<char*>(file_manager->get_data()) + offset;
        std::memcpy(row.cells.data(), read_ptr, num_columns * sizeof(uint64_t));
        
        return row;
    }

} // namespace cw_db