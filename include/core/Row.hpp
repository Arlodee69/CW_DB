#pragma once

#include <vector>
#include <cstdint>

namespace cw_db {

    // Структура, описывающая одну строку таблицы
    struct Row {
        // Поскольку и INT, и STRING (через ID) у нас весят 8 байт, 
        // строка — это просто массив из 8-байтовых чисел.
        std::vector<uint64_t> values;
        
        // Удобный конструктор для напарника
        Row() = default;
        explicit Row(std::vector<uint64_t> vals) : values(std::move(vals)) {}
    };

} // namespace cw_db