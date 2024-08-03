// File: src/lock_free_queue.cpp
#include "nexusdb/lock_free_queue.h"

namespace nexusdb {

template<typename T>
LockFreeQueue<T>::LockFreeQueue() {
    Node* dummy = new Node();
    head_.store(dummy);
    tail_.store(dummy);
}

template<typename T>
LockFreeQueue<T>::~LockFreeQueue() {
    while (Node* old_head = head_.load()) {
        head_.store(old_head->next);
        delete old_head;
    }
}

template<typename T>
void LockFreeQueue<T>::enqueue(T value) {
    Node* new_node = new Node();
    new_node->data = std::make_shared<T>(std::move(value));

    while (true) {
        Node* tail = tail_.load();
        Node* next = tail->next.load();
        if (tail == tail_.load()) {
            if (next == nullptr) {
                if (tail->next.compare_exchange_weak(next, new_node)) {
                    tail_.compare_exchange_strong(tail, new_node);
                    return;
                }
            } else {
                tail_.compare_exchange_strong(tail, next);
            }
        }
    }
}

template<typename T>
std::optional<T> LockFreeQueue<T>::dequeue() {
    while (true) {
        Node* head = head_.load();
        Node* tail = tail_.load();
        Node* next = head->next.load();

        if (head == head_.load()) {
            if (head == tail) {
                if (next == nullptr) {
                    return std::nullopt;
                }
                tail_.compare_exchange_strong(tail, next);
            } else {
                T result = std::move(*next->data);
                if (head_.compare_exchange_weak(head, next)) {
                    delete head;
                    return result;
                }
            }
        }
    }
}

template<typename T>
bool LockFreeQueue<T>::is_empty() const {
    return head_.load() == tail_.load();
}

// Explicit instantiation for common types
template class LockFreeQueue<int>;
template class LockFreeQueue<double>;
template class LockFreeQueue<std::string>;

} // namespace nexusdb