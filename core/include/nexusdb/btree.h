#ifndef NEXUSDB_BTREE_H
#define NEXUSDB_BTREE_H

#include <vector>
#include <memory>
#include <algorithm>
#include <optional>

namespace nexusdb {

template<typename Key, typename Value>
class BTreeNode {
public:
    bool is_leaf;
    std::vector<Key> keys;
    std::vector<Value> values;
    std::vector<std::unique_ptr<BTreeNode>> children;

    BTreeNode(bool leaf = true) : is_leaf(leaf) {}
};

template<typename Key, typename Value>
class BTree {
private:
    std::unique_ptr<BTreeNode<Key, Value>> root;
    size_t degree;

    void split_child(BTreeNode<Key, Value>* parent, int index, BTreeNode<Key, Value>* child);
    void insert_non_full(BTreeNode<Key, Value>* node, const Key& key, const Value& value);

public:
    BTree(size_t degree) : degree(degree), root(std::make_unique<BTreeNode<Key, Value>>()) {}

    void insert(const Key& key, const Value& value);
    std::optional<Value> search(const Key& key) const;
};

// Implementation details...

} // namespace nexusdb

#endif // NEXUSDB_BTREE_H