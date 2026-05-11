#pragma once

#include <string>
#include <cstdint>

namespace cw_db {

    class FileManager {
    
    private:
        int fd = -1;
        size_t file_size = 0;
        void* mapped_data = nullptr;
    
    public:
        explicit FileManager(const std::string& filename, size_t initial_size = 4096);

        ~FileManager();

        FileManager(const FileManager&) = delete;
        FileManager& operator=(const FileManager&) = delete;

        FileManager(FileManager&& other) noexcept;
        FileManager& operator=(FileManager&& other) noexcept;

        void sync();

        void grow_file(size_t new_size);

        void* get_data() const { return mapped_data; }
        size_t get_size() const { return file_size; }
    };

}