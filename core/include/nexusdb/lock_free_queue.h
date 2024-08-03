#ifndef NEXUSDB_LOCK_FREE_QUEUE_H
#define NEXUSDB_LOCK_FREE_QUEUE_H

#include <atomic>
#include <memory>
#include <optional>

namespace nexusdb {

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;
        Node() : next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue();
    ~LockFreeQueue();

    void enqueue(T value);
    std::optional<T> dequeue();
    bool is_empty() const;
};

} // namespace nexusdb

#endif // NEXUSDB_LOCK_FREE_QUEUE_H