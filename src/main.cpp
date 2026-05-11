#include "core/DatabaseEngine.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

using namespace cw_db;

// Вспомогательная функция для вывода строки
void print_row(Table* table, uint64_t id) {
    try {
        auto row = table->select_record(id);
        std::cout << "[ ";
        for (const auto& cell : row) {
            if (std::holds_alternative<uint64_t>(cell)) {
                std::cout << std::get<uint64_t>(cell) << " ";
            } else {
                std::cout << "'" << std::get<std::string>(cell) << "' ";
            }
        }
        std::cout << "]\n";
    } catch (...) {
        std::cout << "[ Запись с ID=" << id << " не найдена или была откачена/удалена ]\n";
    }
}

int main() {
    try {
        std::cout << "=== Запуск полного тестирования Storage Engine ===\n\n";

        // 1. Инициализация движка (создаст логгер и телеметрию автоматически)
        DatabaseEngine engine("./test_db");

        std::vector<ColumnDef> schema = {
            {"id", ColumnType::INT},
            {"name", ColumnType::STRING},
            {"score", ColumnType::INT}
        };

        try {
            engine.create_table("players", schema, 0);
        } catch (...) {
            std::cout << "[Info] Таблица players уже существует, загружаем...\n";
        }

        Table* players = engine.get_table("players");

        // 2. Вставка начальных данных
        std::cout << "--- 1. Базовая вставка ---\n";
        players->insert_record({1ULL, std::string("Алиса"), 100ULL});
        players->insert_record({2ULL, std::string("Боб"), 200ULL});
        print_row(players, 1);
        print_row(players, 2);

        // 3. Тест логгера и телеметрии (Эмулируем работу напарника-парсера)
        std::cout << "\n--- 2. Тестирование Телеметрии и Логов ---\n";
        
        // Успешный запрос (занял 15 мс)
        engine.get_logger()->log_request("client_A", "thread_1", "INSERT ...", 1000, 1015, 200);
        engine.get_telemetry()->record_request(15, false);

        // Ошибочный запрос (занял 2 мс)
        engine.get_logger()->log_request("client_B", "thread_2", "BAD SQL", 1020, 1022, 500);
        engine.get_telemetry()->record_request(2, true);

        std::cout << "Текущая статистика (JSON):\n" 
                  << engine.get_telemetry()->get_metrics_json() << "\n";

        // 4. Тест Машины Времени (REVERT)
        std::cout << "\n--- 3. Тестирование Машины времени (REVERT) ---\n";
        
        // Ждем ровно 1 секунду, чтобы отделить старые события от новых
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // ЗАПОМИНАЕМ ВРЕМЯ (Создаем точку сохранения)
        uint64_t savepoint_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        std::cout << "[!] Точка сохранения создана.\n";

        std::cout << "Злоумышленник ломает базу (меняет очки Алисе и добавляет читера)...\n";
        players->insert_record({3ULL, std::string("Читер"), 9999ULL}); // INSERT
        players->insert_record({1ULL, std::string("Алиса"), 5000ULL}); // UPDATE (Ключ 1 уже есть)

        std::cout << "Испорченные данные:\n";
        print_row(players, 1);
        print_row(players, 3);

        std::cout << "\n[!] АКТИВАЦИЯ REVERT: Откат к точке сохранения!\n";
        players->revert_to(savepoint_time);

        std::cout << "Данные после отката:\n";
        print_row(players, 1); // Должно вернуться к 100
        print_row(players, 3); // Должен исчезнуть

        // 5. Завершение
        engine.sync_all();
        std::cout << "\n=== Все тесты успешно завершены ===\n";

    } catch (const std::exception& e) {
        std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА]: " << e.what() << "\n";
    }

    return 0;
}