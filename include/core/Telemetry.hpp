#pragma once

#include <mutex>
#include <map>
#include <deque>
#include <cstdint>
#include <string>

namespace cw_db {

    // Структура для возврата готовых метрик напарнику
    struct TelemetryStats {
        uint64_t current_rps;
        double avg_rps_10m;
        uint64_t max_rps_10m;
        double avg_processing_time_10s;
        uint64_t error_count_1m;
    };

    class Telemetry {
    private:
        std::mutex mtx;

        // Корзины: [UNIX-секунда] -> [Количество запросов]
        std::map<uint64_t, uint64_t> rps_history;
        
        // Корзины для ошибок: [UNIX-секунда] -> [Количество ошибок]
        std::map<uint64_t, uint64_t> error_history;

        // Для времени обработки за последние 10 сек можно использовать простую очередь 
        // (таймпстемп в мс, длительность в мс)
        std::deque<std::pair<uint64_t, uint64_t>> recent_durations;

        // Внутренний метод для удаления старых данных (чтобы не было утечек памяти)
        void cleanup_old_data(uint64_t current_sec, uint64_t current_ms);

    public:
        Telemetry() = default;

        // Напарник вызывает это после каждого запроса
        void record_request(uint64_t duration_ms, bool is_error);

        // Напарник вызывает это, чтобы получить сводку в реальном времени
        TelemetryStats get_metrics();
        
        // Вспомогательный метод: сразу возвращает красивый JSON-ответ
        std::string get_metrics_json();
    };

} // namespace cw_db