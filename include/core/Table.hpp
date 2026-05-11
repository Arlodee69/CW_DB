#pragma once

#include "storage/RowStore.hpp"
#include "storage/StringPool.hpp"
#include "index/BPlusTree.hpp"
#include <nlohmann/json.hpp> // ОБЯЗАТЕЛЬНО: библиотека для JSON
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <stdexcept>

namespace cw_db {

    using json = nlohmann::json;

    // Типы данных, которые поддерживает наша БД
    enum class ColumnType { INT, STRING };

    // Описание одной колонки (например: "age", INT)
    struct ColumnDef {
        std::string name;
        ColumnType type;
    };

    // Ячейка данных от парсера. 
    using CellValue = std::variant<uint64_t, std::string>;

    // ========================================================================
    // НОВОЕ: Структура для метаданных (нужна для файла schema.json)
    // Она должна быть объявлена ДО класса Table
    // ========================================================================
    struct TableMeta {
        std::string name;
        std::vector<ColumnDef> schema;
        int pk_index;

        // Превращаем структуру в JSON
        json to_json() const {
            json j;
            j["name"] = name;
            j["pk_index"] = pk_index;
            j["columns"] = json::array();
            for (const auto& col : schema) {
                j["columns"].push_back({
                    {"name", col.name},
                    {"type", col.type == ColumnType::INT ? "INT" : "STRING"}
                });
            }
            return j;
        }
    };

    class Table {
    private:
        std::string table_name;
        std::vector<ColumnDef> schema;
        int pk_index; // Индекс колонки, которая является Primary Key

        // Три кита нашего Storage Engine
        std::unique_ptr<RowStore> row_store;
        std::unique_ptr<StringPool> string_pool;
        std::unique_ptr<BPlusTree> bplus_tree;

    public:
        // Конструктор забирает во владение все три компонента
        Table(std::string name, 
              std::vector<ColumnDef> sch, 
              int primary_key_idx,
              std::unique_ptr<RowStore> rs, 
              std::unique_ptr<StringPool> sp, 
              std::unique_ptr<BPlusTree> bpt);
              
        // Возвращает структуру с данными о таблице для сохранения в JSON
        TableMeta get_meta() const {
            return {table_name, schema, pk_index};
        }

        // Вставить новую запись. Парсер передает массив ячеек
        void insert_record(const std::vector<CellValue>& values);

        // Найти запись по Primary Key
        std::vector<CellValue> select_record(uint64_t primary_key);
    };

} // namespace cw_db