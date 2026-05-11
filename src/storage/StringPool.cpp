#include "storage/StringPool.hpp"
#include <cstring>
#include <stdexcept>

namespace cw_db {

    // ============================================================================
    // КОНСТРУКТОР
    // ============================================================================
    StringPool::StringPool(std::unique_ptr<FileManager> fm) 
        : file_manager(std::move(fm)), current_eof(0) 
    {
        // Как только пул создан, мы считываем все существующие строки с диска в хэш-таблицы
        load_from_disk();
    }

    // ============================================================================
    // ЧТЕНИЕ ФАЙЛА ПРИ ЗАПУСКЕ
    // ============================================================================
    void StringPool::load_from_disk() {
        size_t file_size = file_manager->get_size();
        void* raw_data = file_manager->get_data();
        char* char_data = static_cast<char*>(raw_data);

        uint64_t offset = 0;

        // Пока мы не дошли до конца файла
        while (offset < file_size) {
            // Если остался мусор (например, при выравнивании файла), прерываем
            if (offset + sizeof(uint16_t) > file_size) break;

            // 1. Читаем длину строки (2 байта)
            uint16_t length = *reinterpret_cast<uint16_t*>(char_data + offset);
            
            // Если длина 0 (пустая память) — значит это конец реальных данных
            if (length == 0) break; 

            // 2. Считываем саму строку
            std::string str(char_data + offset + sizeof(uint16_t), length);

            // 3. Записываем в кэш (оперативную память)
            string_to_offset[str] = offset;
            offset_to_string[offset] = str;

            // 4. Сдвигаем курсор (Длина числа + Длина самой строки)
            offset += sizeof(uint16_t) + length;
        }

        // Запоминаем, где закончились данные, чтобы писать новые строки отсюда
        current_eof = offset; 
    }

    // ============================================================================
    // ПОЛУЧЕНИЕ ИЛИ ДОБАВЛЕНИЕ СТРОКИ
    // ============================================================================
    uint64_t StringPool::get_or_add_string(const std::string& str) {
        // 1. Дедупликация (String Interning). 
        // Проверяем, есть ли уже такая строка в оперативной памяти.
        auto it = string_to_offset.find(str);
        if (it != string_to_offset.end()) {
            return it->second; // Строка найдена! Отдаем её старый ID, на диск не пишем.
        }

        // 2. Строка новая. Вычисляем, сколько места она займет (2 байта длины + сами символы)
        uint16_t length = static_cast<uint16_t>(str.length());
        size_t required_space = sizeof(uint16_t) + length;

        // 3. Если файл закончился, просим FileManager его увеличить
        if (current_eof + required_space > file_manager->get_size()) {
            // Увеличиваем файл с запасом (+4096 байт), чтобы не дергать ОС слишком часто
            file_manager->grow_file(file_manager->get_size() + 4096);
        }

        // 4. Получаем сырую память и смещаемся в самый конец (current_eof)
        char* append_ptr = static_cast<char*>(file_manager->get_data()) + current_eof;

        // 5. Записываем на жесткий диск (в mmap)
        // Сначала 2 байта длины
        *reinterpret_cast<uint16_t*>(append_ptr) = length;
        
        // Затем сами символы (копируем из std::string прямо в память файла)
        std::memcpy(append_ptr + sizeof(uint16_t), str.c_str(), length);

        // 6. Сохраняем в кэш RAM
        uint64_t new_offset = current_eof;
        string_to_offset[str] = new_offset;
        offset_to_string[new_offset] = str;

        // 7. Сдвигаем курсор конца файла
        current_eof += required_space;

        return new_offset; // Возвращаем ID (смещение) для B+-дерева
    }

    // ============================================================================
    // ВОЗВРАТ СТРОКИ ПО ID
    // ============================================================================
    std::string StringPool::get_string(uint64_t offset) {
        // Ищем в оперативной памяти. Это работает за O(1).
        auto it = offset_to_string.find(offset);
        if (it != offset_to_string.end()) {
            return it->second;
        }
        throw std::runtime_error("String ID not found in pool");
    }

} // namespace cw_db