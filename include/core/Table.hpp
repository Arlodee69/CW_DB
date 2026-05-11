#pragma once

#include "storage/RowStore.hpp"
#include "storage/StringPool.hpp"
#include "index/BPlusTree.hpp"
#include "storage/TransactionLog.hpp" // Подключаем заголовок лога
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <stdexcept>

namespace cw_db {

    using json = nlohmann::json;

    enum class ColumnType { INT, STRING };

    struct ColumnDef {
        std::string name;
        ColumnType type;
    };

    using CellValue = std::variant<uint64_t, std::string>;

    struct TableMeta {
        std::string name;
        std::vector<ColumnDef> schema;
        int pk_index;

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
        int pk_index;

        std::unique_ptr<RowStore> row_store;
        std::unique_ptr<StringPool> string_pool;
        std::unique_ptr<BPlusTree> bplus_tree;
        std::unique_ptr<TransactionLog> tx_log; // Добавлено: четвертый кит хранилища

    public:
        // Конструктор теперь принимает четвертый указатель — на TransactionLog
        Table(std::string name, 
              std::vector<ColumnDef> sch, 
              int primary_key_idx,
              std::unique_ptr<RowStore> rs, 
              std::unique_ptr<StringPool> sp, 
              std::unique_ptr<BPlusTree> bpt,
              std::unique_ptr<TransactionLog> tl);
        
        TableMeta get_meta() const {
            return {table_name, schema, pk_index};
        }

        void insert_record(const std::vector<CellValue>& values);
        std::vector<CellValue> select_record(uint64_t primary_key);
        
        // Добавлено: Метод для отката состояния таблицы во времени
        void revert_to(uint64_t target_timestamp);
    };

} // namespace cw_db