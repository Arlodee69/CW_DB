#include "core/DatabaseEngine.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace cw_db;

void print_row(const std::vector<CellValue>& row) {
    std::cout << "[ ";
    for (const auto& cell : row) {
        if (std::holds_alternative<uint64_t>(cell)) {
            std::cout << std::get<uint64_t>(cell) << " ";
        } else if (std::holds_alternative<std::string>(cell)) {
            std::cout << "'" << std::get<std::string>(cell) << "' ";
        }
    }
    std::cout << "]\n";
}

int main() {
    try {
        std::cout << "=== Запуск Storage Engine ===\n";
        
        // 1. Создаем папку базы данных в текущей директории
        DatabaseEngine engine("./test_db");

        // 2. Создаем схему для таблицы "users" (ID, Имя, Возраст)
        std::vector<ColumnDef> user_schema = {
            {"id", ColumnType::INT},
            {"name", ColumnType::STRING},
            {"age", ColumnType::INT}
        };

        // 3. Создаем таблицу (если ее еще нет). Индекс 0 — это "id" (Primary Key)
        try {
            engine.create_table("users", user_schema, 0);
        } catch (const std::exception& e) {
            std::cout << "[Info] " << e.what() << " (Продолжаем работу)\n";
        }

        // 4. Получаем указатель на таблицу
        Table* users = engine.get_table("users");

        // 5. Вставляем тестовые данные (Парсер будет собирать этот вектор из SQL-запроса)
        std::cout << "\n--- Вставка данных ---\n";
        std::vector<CellValue> row1 = { 1ULL, std::string("Александр"), 25ULL };
        std::vector<CellValue> row2 = { 2ULL, std::string("Елена"), 22ULL };
        std::vector<CellValue> row3 = { 3ULL, std::string("Александр"), 30ULL }; // Дубликат имени для теста пула

        users->insert_record(row1);
        users->insert_record(row2);
        users->insert_record(row3);
        std::cout << "3 записи успешно вставлены!\n";

        // 6. Ищем данные через B+-дерево
        std::cout << "\n--- Поиск данных (SELECT) ---\n";
        
        std::cout << "Поиск ID = 2: ";
        auto result2 = users->select_record(2);
        print_row(result2);

        std::cout << "Поиск ID = 1: ";
        auto result1 = users->select_record(1);
        print_row(result1);

        // 7. Сохраняем все на диск
        engine.sync_all();
        std::cout << "\n=== Работа завершена ===\n";

    } catch (const std::exception& e) {
        std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА]: " << e.what() << "\n";
    }

    return 0;
}