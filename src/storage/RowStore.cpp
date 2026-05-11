#include "storage/RowStore.hpp"
#include <cstring>
#include <stdexcept>

namespace cw_db {

    RowStore::RowStore(std::unique_ptr<FileManager> fm) 
        : file_manager(std::move(fm)) 
    {
        // При запуске базы сразу ставим курсор в конец файла, 
        // чтобы не перезаписать старые данные
        current_eof = file_manager->get_size();
    }

    uint64_t RowStore::append_row(const Row& row) {
        // 1. Считаем размер. Каждая колонка весит ровно 8 байт.
        size_t row_size = row.values.size() * sizeof(uint64_t);

        // 2. Растягиваем файл, если места не хватает
        if (current_eof + row_size > file_manager->get_size()) {
            // Увеличиваем блоками по 4096 байт
            file_manager->grow_file(file_manager->get_size() + 4096);
        }

        // 3. Вычисляем физический адрес в оперативной памяти (mmap)
        char* write_ptr = static_cast<char*>(file_manager->get_data()) + current_eof;

        // 4. Копируем массив значений (std::vector) прямо в память файла
        // row.values.data() дает сырой указатель на элементы вектора
        std::memcpy(write_ptr, row.values.data(), row_size);

        // 5. Запоминаем текущее смещение, чтобы вернуть его
        uint64_t inserted_offset = current_eof;

        // 6. Сдвигаем курсор конца файла
        current_eof += row_size;

        return inserted_offset;
    }

    Row RowStore::get_row(uint64_t offset, size_t num_columns) {
        // Проверка на выход за пределы файла
        size_t row_size = num_columns * sizeof(uint64_t);
        if (offset + row_size > file_manager->get_size()) {
            throw std::out_of_range("Row offset is out of bounds");
        }

        // Вычисляем адрес чтения
        char* read_ptr = static_cast<char*>(file_manager->get_data()) + offset;

        // Создаем пустую строку с нужным количеством колонок
        Row row;
        row.values.resize(num_columns);

        // Копируем байты из файла в наш объект Row
        std::memcpy(row.values.data(), read_ptr, row_size);

        return row;
    }

} // namespace cw_db