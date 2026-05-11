#pragma once

#include "storage/FileManager.hpp"
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace cw_db {
    constexpr size_t PAGE_SIZE = 4096;

    enum class PageType : uint8_t {
        EMPTY = 0,
        META = 1,
        INTERNAL = 2,
        LEAF = 3
    };

    struct NodeHeader;

    class PageManager {
    private:
        std::unique_ptr<FileManager> file_manager;
    
    public:
        explicit PageManager(std::unique_ptr<FileManager> fm);

        void* get_page_raw(uint32_t page_id);
        NodeHeader* get_node_header(uint32_t page_id);
        
        uint32_t allocate_page();
        void sync();
    };
}