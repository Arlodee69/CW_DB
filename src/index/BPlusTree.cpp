#include "index/BPlusTree.hpp"
#include <stdexcept>

namespace cw_db {

    // ============================================================================
    // КОНСТРУКТОР
    // ============================================================================
    BPlusTree::BPlusTree(PageManager* pm, uint32_t root_id) 
        : page_manager(pm), root_page_id(root_id) 
    {
        if (root_page_id == 0) {
            root_page_id = page_manager->allocate_page();
            NodeHeader* header = page_manager->get_node_header(root_page_id);
            header->type = PageType::LEAF;
            header->num_keys = 0;
            header->parent_id = 0;
            header->next_leaf_id = 0;
            header->first_child_id = 0;
        }
    }

    // ============================================================================
    // СПУСК ПО ДЕРЕВУ (Поиск листа для заданного ключа)
    // ============================================================================
    uint32_t BPlusTree::find_leaf_page(KeyType key) {
        uint32_t current_id = root_page_id;

        while (true) {
            void* raw_page = page_manager->get_page_raw(current_id);
            NodeHeader* header = reinterpret_cast<NodeHeader*>(raw_page);

            // Если дошли до листа — возвращаем его ID
            if (header->type == PageType::LEAF) {
                return current_id;
            }

            // Если это внутренний узел, ищем нужный путь
            InternalRecord* records = reinterpret_cast<InternalRecord*>(
                static_cast<char*>(raw_page) + sizeof(NodeHeader)
            );

            // Если ключ меньше самого первого ключа, идем в самую левую ветку
            if (header->num_keys == 0 || key < records[0].key) {
                current_id = header->first_child_id;
                continue;
            }

            // Иначе ищем самый правый ключ, который меньше или равен искомому
            int i = header->num_keys - 1;
            while (i >= 0 && records[i].key > key) {
                i--;
            }
            current_id = records[i].right_child_id;
        }
    }

    // ============================================================================
    // ПУБЛИЧНЫЙ ПОИСК
    // ============================================================================
    bool BPlusTree::search(KeyType key, ValueType& out_value) {
        uint32_t leaf_id = find_leaf_page(key);
        void* raw_page = page_manager->get_page_raw(leaf_id);
        NodeHeader* header = reinterpret_cast<NodeHeader*>(raw_page);
        
        LeafRecord* records = reinterpret_cast<LeafRecord*>(
            static_cast<char*>(raw_page) + sizeof(NodeHeader)
        );

        // Бинарный поиск
        int left = 0, right = header->num_keys - 1; 
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (records[mid].key == key) {
                out_value = records[mid].value; 
                return true;
            } else if (records[mid].key < key) {
                left = mid + 1; 
            } else {
                right = mid - 1; 
            }
        }
        return false; 
    }

    // ============================================================================
    // ПУБЛИЧНАЯ ВСТАВКА
    // ============================================================================
    void BPlusTree::insert(KeyType key, ValueType value) {
        uint32_t leaf_id = find_leaf_page(key);
        NodeHeader* header = page_manager->get_node_header(leaf_id);

        // Если лист переполнен, сначала разбиваем его
        if (header->num_keys >= MAX_LEAF_KEYS) {
            split_leaf_node(leaf_id);
            // После сплита ключ мог уйти в новую половину, поэтому ищем лист заново
            leaf_id = find_leaf_page(key); 
        }

        insert_into_leaf(leaf_id, key, value);
    }

    // ============================================================================
    // ВСТАВКА В ЛИСТ (Сдвиг памяти)
    // ============================================================================
    void BPlusTree::insert_into_leaf(uint32_t leaf_id, KeyType key, ValueType value) {
        void* raw = page_manager->get_page_raw(leaf_id);
        NodeHeader* header = reinterpret_cast<NodeHeader*>(raw);
        LeafRecord* recs = reinterpret_cast<LeafRecord*>(static_cast<char*>(raw) + sizeof(NodeHeader));

        int i = header->num_keys - 1;
        while (i >= 0 && recs[i].key > key) {
            recs[i + 1] = recs[i];
            i--;
        }
        recs[i + 1].key = key;
        recs[i + 1].value = value;
        header->num_keys++;
    }

    // ============================================================================
    // РАЗДЕЛЕНИЕ ЛИСТА
    // ============================================================================
    void BPlusTree::split_leaf_node(uint32_t old_id) {
        void* raw_old = page_manager->get_page_raw(old_id);
        NodeHeader* h_old = reinterpret_cast<NodeHeader*>(raw_old);
        LeafRecord* r_old = reinterpret_cast<LeafRecord*>(static_cast<char*>(raw_old) + sizeof(NodeHeader));

        uint32_t new_id = page_manager->allocate_page();
        void* raw_new = page_manager->get_page_raw(new_id);
        NodeHeader* h_new = reinterpret_cast<NodeHeader*>(raw_new);
        LeafRecord* r_new = reinterpret_cast<LeafRecord*>(static_cast<char*>(raw_new) + sizeof(NodeHeader));

        h_new->type = PageType::LEAF;
        h_new->parent_id = h_old->parent_id;
        h_new->next_leaf_id = h_old->next_leaf_id;
        h_old->next_leaf_id = new_id;

        int split_idx = h_old->num_keys / 2;
        int move_count = h_old->num_keys - split_idx;

        for (int i = 0; i < move_count; i++) {
            r_new[i] = r_old[split_idx + i];
        }

        h_old->num_keys = split_idx;
        h_new->num_keys = move_count;

        KeyType split_key = r_new[0].key;

        if (h_old->parent_id == 0) {
            create_new_root(old_id, split_key, new_id);
        } else {
            // Если родитель заполнен, сначала бьем его, потом вставляем
            NodeHeader* p_head = page_manager->get_node_header(h_old->parent_id);
            if (p_head->num_keys >= MAX_INTERNAL_KEYS) {
                split_internal_node(h_old->parent_id);
                // Найти нового родителя после сплита
                h_old = page_manager->get_node_header(old_id); // Обновляем указатель!
            }
            insert_into_internal(h_old->parent_id, split_key, new_id);
        }
    }

    // ============================================================================
    // ВСТАВКА ВО ВНУТРЕННИЙ УЗЕЛ
    // ============================================================================
    void BPlusTree::insert_into_internal(uint32_t internal_id, KeyType key, uint32_t right_id) {
        void* raw = page_manager->get_page_raw(internal_id);
        NodeHeader* header = reinterpret_cast<NodeHeader*>(raw);
        InternalRecord* recs = reinterpret_cast<InternalRecord*>(static_cast<char*>(raw) + sizeof(NodeHeader));

        int i = header->num_keys - 1;
        while (i >= 0 && recs[i].key > key) {
            recs[i + 1] = recs[i];
            i--;
        }
        recs[i + 1].key = key;
        recs[i + 1].right_child_id = right_id;
        header->num_keys++;
    }

    // ============================================================================
    // РАЗДЕЛЕНИЕ ВНУТРЕННЕГО УЗЛА
    // ============================================================================
    void BPlusTree::split_internal_node(uint32_t old_id) {
        void* raw_old = page_manager->get_page_raw(old_id);
        NodeHeader* h_old = reinterpret_cast<NodeHeader*>(raw_old);
        InternalRecord* r_old = reinterpret_cast<InternalRecord*>(static_cast<char*>(raw_old) + sizeof(NodeHeader));

        uint32_t new_id = page_manager->allocate_page();
        void* raw_new = page_manager->get_page_raw(new_id);
        NodeHeader* h_new = reinterpret_cast<NodeHeader*>(raw_new);
        InternalRecord* r_new = reinterpret_cast<InternalRecord*>(static_cast<char*>(raw_new) + sizeof(NodeHeader));

        h_new->type = PageType::INTERNAL;
        h_new->parent_id = h_old->parent_id;

        int split_idx = h_old->num_keys / 2;
        // Средний ключ поднимается наверх и НЕ остается во внутреннем узле!
        KeyType push_up_key = r_old[split_idx].key; 
        
        // Самый левый ребенок нового узла — это правый ребенок среднего ключа
        h_new->first_child_id = r_old[split_idx].right_child_id;
        
        // Обновляем родителя у перенесенного ребенка
        page_manager->get_node_header(h_new->first_child_id)->parent_id = new_id;

        int move_count = h_old->num_keys - split_idx - 1;
        for (int i = 0; i < move_count; i++) {
            r_new[i] = r_old[split_idx + 1 + i];
            // Обновляем parent_id у всех перенесенных детей
            page_manager->get_node_header(r_new[i].right_child_id)->parent_id = new_id;
        }

        h_old->num_keys = split_idx;
        h_new->num_keys = move_count;

        if (h_old->parent_id == 0) {
            create_new_root(old_id, push_up_key, new_id);
        } else {
            NodeHeader* p_head = page_manager->get_node_header(h_old->parent_id);
            if (p_head->num_keys >= MAX_INTERNAL_KEYS) {
                split_internal_node(h_old->parent_id);
                h_old = page_manager->get_node_header(old_id);
            }
            insert_into_internal(h_old->parent_id, push_up_key, new_id);
        }
    }

    // ============================================================================
    // СОЗДАНИЕ НОВОГО КОРНЯ
    // ============================================================================
    void BPlusTree::create_new_root(uint32_t left_id, KeyType split_key, uint32_t right_id) {
        uint32_t new_root_id = page_manager->allocate_page();
        
        void* raw_root = page_manager->get_page_raw(new_root_id);
        NodeHeader* root_header = reinterpret_cast<NodeHeader*>(raw_root);
        InternalRecord* root_recs = reinterpret_cast<InternalRecord*>(
            static_cast<char*>(raw_root) + sizeof(NodeHeader)
        );

        root_header->type = PageType::INTERNAL;
        root_header->num_keys = 1;
        root_header->parent_id = 0;
        
        // Левый ребенок уходит в first_child_id
        root_header->first_child_id = left_id;
        
        // Правый ребенок уходит в первую запись
        root_recs[0].key = split_key;
        root_recs[0].right_child_id = right_id;

        // Обновляем родителей у детей
        page_manager->get_node_header(left_id)->parent_id = new_root_id;
        page_manager->get_node_header(right_id)->parent_id = new_root_id;

        // Переключаем глобальный корень
        root_page_id = new_root_id;
    }

    // ============================================================================
    // БАЗОВОЕ УДАЛЕНИЕ (Lazy Delete)
    // ============================================================================
    void BPlusTree::remove(KeyType key) {
        uint32_t leaf_id = find_leaf_page(key);
        void* raw = page_manager->get_page_raw(leaf_id);
        NodeHeader* header = reinterpret_cast<NodeHeader*>(raw);
        LeafRecord* recs = reinterpret_cast<LeafRecord*>(static_cast<char*>(raw) + sizeof(NodeHeader));

        int pos = -1;
        for (int i = 0; i < header->num_keys; i++) {
            if (recs[i].key == key) {
                pos = i;
                break;
            }
        }

        if (pos != -1) {
            // Сдвигаем элементы влево, затирая удаленный ключ
            for (int i = pos; i < header->num_keys - 1; i++) {
                recs[i] = recs[i + 1];
            }
            header->num_keys--;
        }
    }

    bool BPlusTree::update_value(KeyType key, ValueType new_value) {
        // 1. Находим нужный лист
        uint32_t leaf_id = find_leaf_page(key);
        void* raw_page = page_manager->get_page_raw(leaf_id);
        
        NodeHeader* header = reinterpret_cast<NodeHeader*>(raw_page);
        // Получаем доступ к массиву записей в листе
        auto* records = reinterpret_cast<LeafRecord*>(
            static_cast<char*>(raw_page) + sizeof(NodeHeader)
        );

        // 2. Бинарный поиск нужного ключа
        int left = 0, right = header->num_keys - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (records[mid].key == key) {
                // Нашли! Перезаписываем старое смещение на новое (или на TOMBSTONE)
                records[mid].value = new_value; 
                return true;
            } else if (records[mid].key < key) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        
        // Ключ не найден
        return false; 
    }

} // namespace cw_db