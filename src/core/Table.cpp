#include "core/Table.hpp"

namespace cw_db {

    // ============================================================================
    // КОНСТРУКТОР
    // ============================================================================
    Table::Table(std::string name, std::vector<ColumnDef> sch, int primary_key_idx,
                 std::unique_ptr<RowStore> rs, std::unique_ptr<StringPool> sp, std::unique_ptr<BPlusTree> bpt)
        : table_name(std::move(name)), 
          schema(std::move(sch)), 
          pk_index(primary_key_idx),
          row_store(std::move(rs)), 
          string_pool(std::move(sp)), 
          bplus_tree(std::move(bpt)) 
    {}

    // ============================================================================
    // INSERT: Собираем данные и распихиваем по файлам
    // ============================================================================
    void Table::insert_record(const std::vector<CellValue>& values) {
        
        // 1. Простейшая валидация: напарник передал столько же значений, сколько у нас колонок?
        if (values.size() != schema.size()) {
            throw std::invalid_argument("Column count mismatch in INSERT");
        }

        // Подготовим массив сырых 8-байтовых чисел для RowStore
        std::vector<uint64_t> raw_row_data(schema.size());
        uint64_t primary_key_value = 0;

        // 2. Обработка каждой ячейки
        for (size_t i = 0; i < schema.size(); i++) {
            
            if (schema[i].type == ColumnType::INT) {
                // Пытаемся достать число. Если напарник ошибся и прислал строку — std::get бросит ошибку.
                uint64_t int_val = std::get<uint64_t>(values[i]);
                raw_row_data[i] = int_val;
                
                // Если это колонка Primary Key, запоминаем её для B+-дерева
                if (i == pk_index) {
                    primary_key_value = int_val;
                }
            } 
            else if (schema[i].type == ColumnType::STRING) {
                // Достаем строку
                std::string str_val = std::get<std::string>(values[i]);
                
                // МАГИЯ: Превращаем строку в 8-байтовый ID через StringPool
                uint64_t string_id = string_pool->get_or_add_string(str_val);
                
                // Кладем этот ID в нашу строку
                raw_row_data[i] = string_id;
            }
        }

        // 3. Сохраняем готовую числовую строку в хранилище (rows.bin)
        Row row(std::move(raw_row_data));
        uint64_t row_offset = row_store->append_row(row);

        // 4. Добавляем запись в индекс (id.idx)
        bplus_tree->insert(primary_key_value, row_offset);
    }

    // ============================================================================
    // SELECT: Ищем в индексе, читаем строку, расшифровываем текст
    // ============================================================================
    std::vector<CellValue> Table::select_record(uint64_t primary_key) {
        
        uint64_t row_offset = 0;

        // 1. Ищем смещение в B+-дереве
        if (!bplus_tree->search(primary_key, row_offset)) {
            // Если функция вернула false — ключа нет
            throw std::runtime_error("Record not found");
        }

        // 2. Читаем сырые 8-байтовые числа из хранилища строк
        Row raw_row = row_store->get_row(row_offset, schema.size());

        // 3. Подготавливаем красивый ответ для напарника
        std::vector<CellValue> result(schema.size());

        for (size_t i = 0; i < schema.size(); i++) {
            if (schema[i].type == ColumnType::INT) {
                // Число отдаем как есть
                result[i] = raw_row.values[i];
            } 
            else if (schema[i].type == ColumnType::STRING) {
                // Достаем ID строки
                uint64_t string_id = raw_row.values[i];
                
                // МАГИЯ: Расшифровываем ID обратно в текст через StringPool
                std::string original_text = string_pool->get_string(string_id);
                
                // Кладем текст в ответ
                result[i] = original_text;
            }
        }

        return result;
    }

} // namespace cw_db