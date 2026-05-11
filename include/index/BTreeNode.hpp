#pragma once

#include "storage/PageManager.hpp"
#include <cstdint>

namespace cw_db {

    using KeyType = uint64_t;
    using ValueType = uint64_t; // Смещение в файле строк или данных

    // Жесткая упаковка заголовка (15 байт)
    #pragma pack(push, 1)
    struct NodeHeader {
        PageType type;           // LEAF или INTERNAL (1 байт)
        uint16_t num_keys;       // Текущее число ключей (2 байта)
        uint32_t parent_id;      // ID родителя (4 байта)
        uint32_t next_leaf_id;   // Для листьев: ID следующего листа (4 байта)
        uint32_t first_child_id; // Для внутренних: самый левый указатель (4 байта)
    };
    #pragma pack(pop)

    // Запись для листа (16 байт)
    #pragma pack(push, 1)
    struct LeafRecord {
        KeyType key;
        ValueType value;
    };
    #pragma pack(pop)

    // Запись для внутреннего узла (12 байт)
    #pragma pack(push, 1)
    struct InternalRecord {
        KeyType key;
        uint32_t right_child_id; // Указатель на страницу справа от ключа
    };
    #pragma pack(pop)

    // Константы вместимости (исходя из страницы 4096 байт и заголовка 15 байт)
    constexpr uint16_t MAX_LEAF_KEYS = (PAGE_SIZE - sizeof(NodeHeader)) / sizeof(LeafRecord);       // ~255
    constexpr uint16_t MAX_INTERNAL_KEYS = (PAGE_SIZE - sizeof(NodeHeader)) / sizeof(InternalRecord); // ~340

} // namespace cw_db