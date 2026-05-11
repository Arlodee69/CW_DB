#include "storage/PageManager.hpp"
#include "index/BTreeNode.hpp" 
#include <stdexcept>
#include <cstring>

namespace cw_db {
    PageManager::PageManager(std::unique_ptr<FileManager> fm)
        : file_manager(std::move(fm)) {}
    
    void* PageManager::get_page_raw(uint32_t page_id) {
        size_t offset = page_id * PAGE_SIZE;

        if (offset >= file_manager->get_size()) {
            throw std::out_of_range("За доступными пределами");
        }

        return static_cast<char*>(file_manager->get_data()) + offset;
    }

    NodeHeader* PageManager::get_node_header(uint32_t page_id) {
        void* raw_page = get_page_raw(page_id);
        return reinterpret_cast<NodeHeader*>(raw_page);
    }

    uint32_t PageManager::allocate_page() {
        uint32_t new_page_id = file_manager->get_size() / PAGE_SIZE;
        file_manager->grow_file(file_manager->get_size() + PAGE_SIZE);

        NodeHeader* new_header = get_node_header(new_page_id);
        new_header->type = PageType::EMPTY; 
        new_header->parent_id = 0;
        new_header->next_leaf_id = 0;

        return new_page_id;
    }

    void PageManager::sync() {
        file_manager->sync();
    }
}