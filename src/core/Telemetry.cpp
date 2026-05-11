#include "core/Telemetry.hpp"
#include <chrono>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace cw_db {

    using json = nlohmann::json;

    // Вспомогательная функция времени
    inline uint64_t get_time_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void Telemetry::cleanup_old_data(uint64_t current_sec, uint64_t current_ms) {
        // Оставляем RPS только за последние 10 минут (600 секунд)
        uint64_t cutoff_10m = current_sec > 600 ? current_sec - 600 : 0;
        rps_history.erase(rps_history.begin(), rps_history.lower_bound(cutoff_10m));

        // Оставляем ошибки только за последнюю 1 минуту (60 секунд)
        uint64_t cutoff_1m = current_sec > 60 ? current_sec - 60 : 0;
        error_history.erase(error_history.begin(), error_history.lower_bound(cutoff_1m));

        // Оставляем тайминги только за последние 10 секунд (10000 мс)
        uint64_t cutoff_10s_ms = current_ms > 10000 ? current_ms - 10000 : 0;
        while (!recent_durations.empty() && recent_durations.front().first < cutoff_10s_ms) {
            recent_durations.pop_front();
        }
    }

    void Telemetry::record_request(uint64_t duration_ms, bool is_error) {
        uint64_t current_ms = get_time_ms();
        uint64_t current_sec = current_ms / 1000;

        std::lock_guard<std::mutex> lock(mtx);
        
        cleanup_old_data(current_sec, current_ms);

        // Увеличиваем счетчики
        rps_history[current_sec]++;
        if (is_error) {
            error_history[current_sec]++;
        }
        
        recent_durations.push_back({current_ms, duration_ms});
    }

    TelemetryStats Telemetry::get_metrics() {
        uint64_t current_ms = get_time_ms();
        uint64_t current_sec = current_ms / 1000;

        std::lock_guard<std::mutex> lock(mtx);
        cleanup_old_data(current_sec, current_ms);

        TelemetryStats stats = {0, 0.0, 0, 0.0, 0};

        // 1. Текущий RPS (за текущую секунду)
        if (rps_history.find(current_sec) != rps_history.end()) {
            stats.current_rps = rps_history[current_sec];
        }

        // 2. Статистика RPS за 10 минут
        uint64_t total_requests_10m = 0;
        for (const auto& [sec, count] : rps_history) {
            total_requests_10m += count;
            if (count > stats.max_rps_10m) {
                stats.max_rps_10m = count;
            }
        }
        // Защита от деления на ноль, считаем среднее по количеству активных секунд (или фиксированно / 600)
        double active_seconds = rps_history.size() > 0 ? static_cast<double>(rps_history.size()) : 1.0;
        stats.avg_rps_10m = static_cast<double>(total_requests_10m) / active_seconds;

        // 3. Ошибки за 1 минуту
        for (const auto& [sec, count] : error_history) {
            stats.error_count_1m += count;
        }

        // 4. Среднее время за 10 сек
        if (!recent_durations.empty()) {
            uint64_t total_time = 0;
            for (const auto& item : recent_durations) {
                total_time += item.second;
            }
            stats.avg_processing_time_10s = static_cast<double>(total_time) / recent_durations.size();
        }

        return stats;
    }

    std::string Telemetry::get_metrics_json() {
        TelemetryStats stats = get_metrics();
        json j;
        j["current_rps"] = stats.current_rps;
        j["avg_rps_10m"] = stats.avg_rps_10m;
        j["max_rps_10m"] = stats.max_rps_10m;
        j["avg_processing_time_10s_ms"] = stats.avg_processing_time_10s;
        j["error_count_1m"] = stats.error_count_1m;
        return j.dump(4);
    }

} // namespace cw_db