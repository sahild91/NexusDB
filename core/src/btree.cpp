#include "nexusdb/btree.h"
#include <optional>

namespace nexusdb {

template<typename Key, typename Value>
void BTree<Key, Value>::insert(const Key& key, const Value& value) {
    if (root->keys.size() == 2 * degree - 1) {
        auto new_root = std::make_unique<BTreeNode<Key, Value>>(false);
        new_root->children.push_back(std::move(root));
        root = std::move(new_root);
        split_child(root.get(), 0, root->children[0].get());
    }
    insert_non_full(root.get(), key, value);
}

template<typename Key, typename Value>
void BTree<Key, Value>::split_child(BTreeNode<Key, Value>* parent, int index, BTreeNode<Key, Value>* child) {
    auto new_child = std::make_unique<BTreeNode<Key, Value>>(child->is_leaf);
    
    for (int j = 0; j < degree - 1; j++) {
        new_child->keys.push_back(child->keys[j + degree]);
        if (!child->is_leaf) {
            new_child->children.push_back(std::move(child->children[j + degree]));
        }
    }
    
    if (!child->is_leaf) {
        new_child->children.push_back(std::move(child->children[2 * degree - 1]));
    }
    
    child->keys.resize(degree - 1);
    child->children.resize(degree);
    
    parent->children.insert(parent->children.begin() + index + 1, std::move(new_child));
    parent->keys.insert(parent->keys.begin() + index, child->keys[degree - 1]);
}

template<typename Key, typename Value>
void BTree<Key, Value>::insert_non_full(BTreeNode<Key, Value>* node, const Key& key, const Value& value) {
    int i = node->keys.size() - 1;
    
    if (node->is_leaf) {
        node->keys.push_back(key);
        node->values.push_back(value);
        
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
    } else {
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        
        if (node->children[i]->keys.size() == 2 * degree - 1) {
            split_child(node, i, node->children[i].get());
            if (key > node->keys[i]) {
                i++;
            }
        }
        insert_non_full(node->children[i].get(), key, value);
    }
}

template<typename Key, typename Value>
std::optional<Value> BTree<Key, Value>::search(const Key& key) const {
    BTreeNode<Key, Value>* current = root.get();
    while (current != nullptr) {
        auto it = std::lower_bound(current->keys.begin(), current->keys.end(), key);
        if (it != current->keys.end() && *it == key) {
            return current->values[it - current->keys.begin()];
        }
        if (current->is_leaf) {
            return std::nullopt;
        }
        current = current->children[it - current->keys.begin()].get();
    }
    return std::nullopt;
}

} // namespace nexusdb