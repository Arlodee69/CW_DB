#pragma once

#include "core/Table.hpp"
#include <nlohmann/json.hpp> // Не забудь библиотеку JSON!
#include <filesystem>
#include <unordered_map>
#include <string>
#include <memory>
#include "core/AccessLogger.hpp"
#include "core/Telemetry.hpp"

namespace cw_db {
    using json = nlohmann::json;

    class DatabaseEngine {
    private:
        std::string db_directory;
        std::unordered_map<std::string, std::unique_ptr<Table>> tables;
        std::unique_ptr<AccessLogger> access_logger;
        std::unique_ptr<Telemetry> telemetry;

        // Внутренние методы
        std::string get_file_path(const std::string& table_name, const std::string& extension);
        void load_existing_table(const std::string& name, const std::vector<ColumnDef>& schema, int pk_index);
        void save_catalog();

    public:
        explicit DatabaseEngine(const std::string& directory_path);
        
        void create_table(const std::string& name, const std::vector<ColumnDef>& schema, int pk_index);
        Table* get_table(const std::string& name);
        void sync_all();

        AccessLogger* get_logger() const { return access_logger.get(); }
        Telemetry* get_telemetry() const { return telemetry.get(); }
    };
}