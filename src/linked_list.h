#pragma once
#include <cstddef>

// Singly linked list (used for notifications)
template <typename T>
class LinkedList {
    struct Node { T val; Node* next; };
    Node* head = nullptr;
    Node* tail = nullptr;
    size_t n = 0;

public:
    LinkedList() = default;
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;
    ~LinkedList() { clear(); }

    void pushBack(const T& v) {
        Node* node = new Node{v, nullptr};
        if (tail) tail->next = node; else head = node;
        tail = node;
        ++n;
    }
    bool popFront(T& out) {
        if (!head) return false;
        Node* t = head;
        out = t->val;
        head = head->next;
        if (!head) tail = nullptr;
        delete t;
        --n;
        return true;
    }
    bool empty() const { return n == 0; }
    size_t size() const { return n; }
    template <typename F>
    void forEach(F f) const {
        for (Node* c = head; c; c = c->next) f(c->val);
    }
    void clear() { T tmp; while (popFront(tmp)) {} }
};

// FIFO queue built on the linked list (used by BFS)
template <typename T>
class Queue {
    LinkedList<T> list;
public:
    void push(const T& v) { list.pushBack(v); }
    bool pop(T& out) { return list.popFront(out); }
    bool empty() const { return list.empty(); }
    size_t size() const { return list.size(); }
};
