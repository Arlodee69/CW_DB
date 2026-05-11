#include "core/DatabaseEngine.hpp"
#include <fstream>
#include <iostream>

namespace cw_db {

// ============================================================================
    // КОНСТРУКТОР: Оживляет базу при запуске
    // ============================================================================
    DatabaseEngine::DatabaseEngine(const std::string& directory_path) 
        : db_directory(directory_path) 
    {
        // 1. Гарантируем, что папка существует
        if (!std::filesystem::exists(db_directory)) {
            std::filesystem::create_directories(db_directory);
        } 
        
        // 2. БЕЗУСЛОВНО создаем логгер (папка теперь точно есть)
        access_logger = std::make_unique<AccessLogger>(db_directory + "/access.log");
        telemetry = std::make_unique<Telemetry>();
        
        // 3. Загружаем каталог таблиц, если он есть
        std::string schema_path = db_directory + "/schema.json";
        if (std::filesystem::exists(schema_path)) {
            try {
                std::ifstream file(schema_path);
                json catalog_json;
                file >> catalog_json;

                for (auto& table_json : catalog_json) {
                    std::string name = table_json["name"];
                    int pk_idx = table_json["pk_index"];
                    
                    std::vector<ColumnDef> schema;
                    for (auto& col_j : table_json["columns"]) {
                        ColumnType type = (col_j["type"] == "INT") ? ColumnType::INT : ColumnType::STRING;
                        schema.push_back({col_j["name"], type});
                    }

                    load_existing_table(name, schema, pk_idx);
                }
            } catch (const std::exception& e) {
                std::cerr << "[Error] Ошибка загрузки каталога: " << e.what() << "\n";
            }
        }
    }

    // ============================================================================
    // СОЗДАНИЕ НОВОЙ ТАБЛИЦЫ
    // ============================================================================
    void DatabaseEngine::create_table(const std::string& name, const std::vector<ColumnDef>& schema, int pk_index) {
        if (tables.find(name) != tables.end()) {
            throw std::runtime_error("Таблица '" + name + "' уже существует!");
        }

        // 1. Создаем физические файлы (FileManager сам создаст их на диске)
        auto fm_rows = std::make_unique<FileManager>(get_file_path(name, "_rows.bin"));
        auto fm_strings = std::make_unique<FileManager>(get_file_path(name, "_strings.bin"));
        auto fm_index = std::make_unique<FileManager>(get_file_path(name, "_index.idx"));
        auto fm_log = std::make_unique<FileManager>(get_file_path(name, "_log.bin")); // ДОБАВЛЕНО: Файл лога

        // 2. Инициализируем логические слои
        auto row_store = std::make_unique<RowStore>(std::move(fm_rows));
        auto string_pool = std::make_unique<StringPool>(std::move(fm_strings));
        auto page_manager = new PageManager(std::move(fm_index));
        auto bplus_tree = std::make_unique<BPlusTree>(page_manager, 0); 
        auto tx_log = std::make_unique<TransactionLog>(std::move(fm_log)); // ДОБАВЛЕНО: Журнал транзакций

        // 3. Собираем объект Table (теперь с логом)
        tables[name] = std::make_unique<Table>(
            name, schema, pk_index, 
            std::move(row_store), std::move(string_pool), std::move(bplus_tree), std::move(tx_log)
        );

        // 4. Записываем новую таблицу в schema.json
        save_catalog();
        
        std::cout << "[DB Engine] Создана новая таблица: " << name << "\n";
    }

    // ============================================================================
    // ЗАГРУЗКА СУЩЕСТВУЮЩЕЙ ТАБЛИЦЫ
    // ============================================================================
    void DatabaseEngine::load_existing_table(const std::string& name, const std::vector<ColumnDef>& schema, int pk_index) {
        // Подхватываем существующие файлы
        auto fm_rows = std::make_unique<FileManager>(get_file_path(name, "_rows.bin"));
        auto fm_strings = std::make_unique<FileManager>(get_file_path(name, "_strings.bin"));
        auto fm_index = std::make_unique<FileManager>(get_file_path(name, "_index.idx"));
        auto fm_log = std::make_unique<FileManager>(get_file_path(name, "_log.bin")); // ДОБАВЛЕНО: Подхватываем старый лог

        auto row_store = std::make_unique<RowStore>(std::move(fm_rows));
        auto string_pool = std::make_unique<StringPool>(std::move(fm_strings));
        auto page_manager = new PageManager(std::move(fm_index));
        auto bplus_tree = std::make_unique<BPlusTree>(page_manager, 0); 
        auto tx_log = std::make_unique<TransactionLog>(std::move(fm_log)); // ДОБАВЛЕНО: Восстанавливаем журнал

        tables[name] = std::make_unique<Table>(
            name, schema, pk_index, 
            std::move(row_store), std::move(string_pool), std::move(bplus_tree), std::move(tx_log)
        );
        
        std::cout << "[DB Engine] Таблица подгружена: " << name << "\n";
    }

    // ============================================================================
    // СОХРАНЕНИЕ КАТАЛОГА
    // ============================================================================
    void DatabaseEngine::save_catalog() {
        json catalog_json = json::array();
        
        for (auto const& [name, table] : tables) {
            catalog_json.push_back(table->get_meta().to_json());
        }

        std::ofstream file(db_directory + "/schema.json");
        if (file.is_open()) {
            file << catalog_json.dump(4);
            file.close();
        }
    }

    // ============================================================================
    // СИНХРОНИЗАЦИЯ
    // ============================================================================
    void DatabaseEngine::sync_all() {
        save_catalog();
        std::cout << "[DB Engine] Все метаданные и структуры синхронизированы.\n";
    }

    // Вспомогательные методы
    std::string DatabaseEngine::get_file_path(const std::string& table_name, const std::string& extension) {
        return db_directory + "/" + table_name + extension;
    }

    Table* DatabaseEngine::get_table(const std::string& name) {
        auto it = tables.find(name);
        if (it != tables.end()) return it->second.get();
        throw std::runtime_error("Таблица '" + name + "' не найдена!");
    }

} // namespace cw_db