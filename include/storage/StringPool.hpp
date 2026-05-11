#pragma once

#include "storage/FileManager.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace cw_db {

    class StringPool {
    private:
        std::unique_ptr<FileManager> file_manager;
        
        // Два словаря в оперативной памяти для мгновенного поиска
        // 1. Найти ID (смещение) по строке (нужно при INSERT)
        std::unordered_map<std::string, uint64_t> string_to_offset;
        
        // 2. Найти строку по ID (нужно при SELECT для вывода на экран)
        std::unordered_map<uint64_t, std::string> offset_to_string;

        // Текущий конец файла (куда мы будем дописывать новые строки)
        uint64_t current_eof;

        // Вспомогательный метод: прочитать весь файл при запуске БД
        void load_from_disk();

    public:
        // Конструктор принимает файловый менеджер (файл strings.bin)
        explicit StringPool(std::unique_ptr<FileManager> fm);

        // Главный метод 1: Даем строку, получаем её уникальный ID (смещение).
        // Если строка уже есть — вернет старый ID. Если нет — запишет на диск и вернет новый.
        uint64_t get_or_add_string(const std::string& str);

        // Главный метод 2: Даем ID, получаем строку обратно (для парсера напарника).
        std::string get_string(uint64_t offset);
    };

} // namespace cw_db