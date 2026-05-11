#include "core/Table.hpp"
#include <iostream>
#include <chrono>

namespace cw_db {

    // Вспомогательная функция для получения текущего времени в миллисекундах
    uint64_t get_current_time_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    // Обновленный конструктор с TransactionLog
    Table::Table(std::string name, std::vector<ColumnDef> sch, int primary_key_idx,
                 std::unique_ptr<RowStore> rs, std::unique_ptr<StringPool> sp, 
                 std::unique_ptr<BPlusTree> bpt, std::unique_ptr<TransactionLog> tl)
        : table_name(std::move(name)), schema(std::move(sch)), pk_index(primary_key_idx),
          row_store(std::move(rs)), string_pool(std::move(sp)), 
          bplus_tree(std::move(bpt)), tx_log(std::move(tl)) {}

    void Table::insert_record(const std::vector<CellValue>& values) {
        if (values.size() != schema.size()) {
            throw std::invalid_argument("Количество значений не совпадает со схемой таблицы");
        }

        uint64_t primary_key_value = 0;
        if (std::holds_alternative<uint64_t>(values[pk_index])) {
            primary_key_value = std::get<uint64_t>(values[pk_index]);
        } else {
            throw std::invalid_argument("Primary Key должен быть числом (uint64_t)");
        }

        Row new_row;
        for (size_t i = 0; i < values.size(); ++i) {
            if (schema[i].type == ColumnType::INT) {
                new_row.cells.push_back(std::get<uint64_t>(values[i]));
            } else {
                std::string text = std::get<std::string>(values[i]);
                uint64_t string_id = string_pool->get_or_add_string(text);
                new_row.cells.push_back(string_id);
            }
        }

        uint64_t row_offset = row_store->append_row(new_row);

        // --- ЛОГИКА ЖУРНАЛИРОВАНИЯ ---
        uint64_t old_offset = 0;
        bool existed = bplus_tree->search(primary_key_value, old_offset);

        if (existed && old_offset != TOMBSTONE) {
            // Ключ уже был (это UPDATE)
            bplus_tree->update_value(primary_key_value, row_offset);
            tx_log->append_log(get_current_time_ms(), OpType::UPDATE, primary_key_value, old_offset);
        } else {
            // Ключа не было (это INSERT)
            if (existed && old_offset == TOMBSTONE) {
                // Воскрешаем удаленный ключ
                bplus_tree->update_value(primary_key_value, row_offset);
            } else {
                // Создаем новый
                bplus_tree->insert(primary_key_value, row_offset);
            }
            tx_log->append_log(get_current_time_ms(), OpType::INSERT, primary_key_value, 0);
        }
    }

    std::vector<CellValue> Table::select_record(uint64_t primary_key) {
        uint64_t row_offset = 0;
        
        // Добавлена проверка на TOMBSTONE
        if (!bplus_tree->search(primary_key, row_offset) || row_offset == TOMBSTONE) {
            throw std::runtime_error("Запись не найдена (или была удалена/откачена)");
        }

        Row row = row_store->read_row(row_offset, schema.size());
        std::vector<CellValue> result;

        for (size_t i = 0; i < schema.size(); ++i) {
            if (schema[i].type == ColumnType::INT) {
                result.push_back(row.cells[i]);
            } else {
                result.push_back(string_pool->get_string(row.cells[i]));
            }
        }

        return result;
    }

    // ============================================================================
    // МАШИНА ВРЕМЕНИ: Откат базы данных к определенному времени
    // ============================================================================
    void Table::revert_to(uint64_t target_timestamp) {
        // Читаем журнал задом наперед
        std::vector<LogEntry> history = tx_log->read_logs_backwards_until(target_timestamp);

        // Отменяем действия
        for (const auto& entry : history) {
            if (entry.type == OpType::INSERT) {
                // Если мы вставили данные, откат — это "удаление" (прячем ключ)
                bplus_tree->update_value(entry.pk, TOMBSTONE);
            } 
            else if (entry.type == OpType::UPDATE || entry.type == OpType::DELETE) {
                // Если мы изменили данные, возвращаем указатель на старое место в файле
                bplus_tree->update_value(entry.pk, entry.old_offset);
            }
        }
        
        std::cout << "[Table " << table_name << "] Успешно выполнен откат " 
                  << history.size() << " операций.\n";
    }

} // namespace cw_db