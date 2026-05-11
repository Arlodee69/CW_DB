#pragma once

#include "storage/PageManager.hpp"
#include "index/BTreeNode.hpp"
#include <limits>

namespace cw_db {

    constexpr uint64_t TOMBSTONE = std::numeric_limits<uint64_t>::max();

    class BPlusTree {
    private:
        PageManager* page_manager;
        uint32_t root_page_id;
        

        // Внутренние механизмы навигации и вставки
        uint32_t find_leaf_page(KeyType key);
        
        void insert_into_leaf(uint32_t leaf_id, KeyType key, ValueType value);
        void split_leaf_node(uint32_t leaf_id);
        
        void insert_into_internal(uint32_t internal_id, KeyType key, uint32_t right_child_id);
        void split_internal_node(uint32_t internal_id);
        
        void create_new_root(uint32_t left_id, KeyType split_key, uint32_t right_id);

    public:
        BPlusTree(PageManager* pm, uint32_t root_id);
        bool update_value(KeyType key, ValueType new_value);
        bool search(KeyType key, ValueType& out_value);
        void insert(KeyType key, ValueType value);
        void remove(KeyType key); // Базовое удаление
    };

} // namespace cw_db