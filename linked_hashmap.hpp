/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    
template<class Key, class T, class Hash = std::hash<Key>, class Equal = std::equal_to<Key> >
class linked_hashmap {
private:
    struct Node;
public:
    typedef pair<const Key, T> value_type;
 
private:
    struct Node {
        value_type* data_ptr;
        Node* prev;
        Node* next;
        Node* hash_next;
        
        Node() : data_ptr(nullptr), prev(nullptr), next(nullptr), hash_next(nullptr) {}
        Node(const Key& k, const T& v) : data_ptr(new value_type(k, v)), prev(nullptr), next(nullptr), hash_next(nullptr) {}
        Node(const value_type& val) : data_ptr(new value_type(val)), prev(nullptr), next(nullptr), hash_next(nullptr) {}
        ~Node() {
            delete data_ptr;
        }
    };
    
    Node* head_;
    Node* tail_;
    Node** buckets_;
    size_t bucket_count_;
    size_t size_;
    Hash hash_fn_;
    Equal eq_fn_;
    
    static constexpr double LOAD_FACTOR = 0.75;
    static constexpr size_t INITIAL_BUCKET_COUNT = 16;
    
    size_t get_bucket_index(const Key& key) const {
        return hash_fn_(key) % bucket_count_;
    }
    
    void init_buckets() {
        buckets_ = new Node*[bucket_count_]();
        for (size_t i = 0; i < bucket_count_; ++i) {
            buckets_[i] = nullptr;
        }
    }
    
    void rehash(size_t new_count) {
        Node** old_buckets = buckets_;
        size_t old_count = bucket_count_;
        
        bucket_count_ = new_count;
        buckets_ = new Node*[bucket_count_]();
        for (size_t i = 0; i < bucket_count_; ++i) {
            buckets_[i] = nullptr;
        }
        
        for (Node* node = head_->next; node != tail_; node = node->next) {
            size_t idx = get_bucket_index(node->data_ptr->first);
            node->hash_next = buckets_[idx];
            buckets_[idx] = node;
        }
        
        delete[] old_buckets;
    }
    
    Node* find_node(const Key& key) const {
        size_t idx = get_bucket_index(key);
        for (Node* node = buckets_[idx]; node != nullptr; node = node->hash_next) {
            if (eq_fn_(node->data_ptr->first, key)) {
                return node;
            }
        }
        return nullptr;
    }
    
    void unlink_node(Node* node) {
        size_t idx = get_bucket_index(node->data_ptr->first);
        if (buckets_[idx] == node) {
            buckets_[idx] = node->hash_next;
        } else {
            Node* prev_bucket = buckets_[idx];
            while (prev_bucket != nullptr && prev_bucket->hash_next != node) {
                prev_bucket = prev_bucket->hash_next;
            }
            if (prev_bucket != nullptr) {
                prev_bucket->hash_next = node->hash_next;
            }
        }
        
        node->prev->next = node->next;
        node->next->prev = node->prev;
        
        delete node;
        --size_;
    }
    
public:
    class const_iterator;
    class iterator {
    private:
        Node* node_;
        const linked_hashmap* map_;
        
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::forward_iterator_tag;

        iterator() : node_(nullptr), map_(nullptr) {}
        iterator(Node* node, const linked_hashmap* map) : node_(node), map_(map) {}
        iterator(const iterator& other) : node_(other.node_), map_(other.map_) {}
        
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        iterator& operator++() {
            if (node_ == nullptr || node_ == map_->tail_) {
                throw invalid_iterator();
            }
            node_ = node_->next;
            return *this;
        }
        
        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }
        
        iterator& operator--() {
            if (node_ == nullptr || node_->prev == map_->head_) {
                throw invalid_iterator();
            }
            node_ = node_->prev;
            return *this;
        }
        
        value_type& operator*() const {
            if (node_ == nullptr || node_ == map_->head_ || node_ == map_->tail_) {
                throw invalid_iterator();
            }
            return *(node_->data_ptr);
        }
        
        bool operator==(const iterator& rhs) const {
            return node_ == rhs.node_;
        }
        
        bool operator==(const const_iterator& rhs) const {
            return node_ == rhs.get_node();
        }
        
        bool operator!=(const iterator& rhs) const {
            return node_ != rhs.node_;
        }
        
        bool operator!=(const const_iterator& rhs) const {
            return node_ != rhs.get_node();
        }

        value_type* operator->() const noexcept {
            return node_->data_ptr;
        }
        
        Node* get_node() const { return node_; }
        const linked_hashmap* get_map() const { return map_; }
    };
 
    class const_iterator {
    private:
        Node* node_;
        const linked_hashmap* map_;
    public:
        friend class iterator;
        
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::forward_iterator_tag;
        
        const_iterator() : node_(nullptr), map_(nullptr) {}
        const_iterator(Node* node, const linked_hashmap* map) : node_(node), map_(map) {}
        const_iterator(const const_iterator& other) : node_(other.node_), map_(other.map_) {}
        const_iterator(const iterator& other) : node_(other.get_node()), map_(other.get_map()) {}
        
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        const_iterator& operator++() {
            if (node_ == nullptr || node_ == map_->tail_) {
                throw invalid_iterator();
            }
            node_ = node_->next;
            return *this;
        }
        
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }
        
        const_iterator& operator--() {
            if (node_ == nullptr || node_->prev == map_->head_) {
                throw invalid_iterator();
            }
            node_ = node_->prev;
            return *this;
        }
        
        const value_type& operator*() const {
            if (node_ == nullptr || node_ == map_->head_ || node_ == map_->tail_) {
                throw invalid_iterator();
            }
            return *(node_->data_ptr);
        }
        
        bool operator==(const const_iterator& rhs) const {
            return node_ == rhs.node_;
        }
        
        bool operator==(const iterator& rhs) const {
            return node_ == rhs.get_node();
        }
        
        bool operator!=(const const_iterator& rhs) const {
            return node_ != rhs.node_;
        }
        
        bool operator!=(const iterator& rhs) const {
            return node_ != rhs.get_node();
        }
        
        const value_type* operator->() const noexcept {
            return node_->data_ptr;
        }
        
        Node* get_node() const { return node_; }
        const linked_hashmap* get_map() const { return map_; }
    };
 
    linked_hashmap() : bucket_count_(INITIAL_BUCKET_COUNT), size_(0), hash_fn_(), eq_fn_() {
        head_ = new Node();
        tail_ = new Node();
        head_->next = tail_;
        tail_->prev = head_;
        init_buckets();
    }
    
    linked_hashmap(const linked_hashmap& other) : bucket_count_(INITIAL_BUCKET_COUNT), size_(0), hash_fn_(other.hash_fn_), eq_fn_(other.eq_fn_) {
        head_ = new Node();
        tail_ = new Node();
        head_->next = tail_;
        tail_->prev = head_;
        init_buckets();
        
        for (Node* node = other.head_->next; node != other.tail_; node = node->next) {
            insert(*(node->data_ptr));
        }
    }
 
    linked_hashmap& operator=(const linked_hashmap& other) {
        if (this == &other) return *this;
        
        clear();
        hash_fn_ = other.hash_fn_;
        eq_fn_ = other.eq_fn_;
        
        for (Node* node = other.head_->next; node != other.tail_; node = node->next) {
            insert(*(node->data_ptr));
        }
        return *this;
    }
 
    ~linked_hashmap() {
        clear();
        delete head_;
        delete tail_;
        delete[] buckets_;
    }
 
    T& at(const Key& key) {
        Node* node = find_node(key);
        if (node == nullptr) {
            throw index_out_of_bound();
        }
        return node->data_ptr->second;
    }
    
    const T& at(const Key& key) const {
        Node* node = find_node(key);
        if (node == nullptr) {
            throw index_out_of_bound();
        }
        return node->data_ptr->second;
    }
 
    T& operator[](const Key& key) {
        Node* node = find_node(key);
        if (node != nullptr) {
            return node->data_ptr->second;
        }
        
        if (size_ >= static_cast<size_t>(bucket_count_ * LOAD_FACTOR)) {
            rehash(bucket_count_ * 2);
        }
        
        Node* new_node = new Node(key, T());
        
        size_t idx = get_bucket_index(key);
        new_node->hash_next = buckets_[idx];
        buckets_[idx] = new_node;
        
        new_node->prev = tail_->prev;
        new_node->next = tail_;
        tail_->prev->next = new_node;
        tail_->prev = new_node;
        
        ++size_;
        return new_node->data_ptr->second;
    }
 
    const T& operator[](const Key& key) const {
        Node* node = find_node(key);
        if (node == nullptr) {
            throw index_out_of_bound();
        }
        return node->data_ptr->second;
    }
 
    iterator begin() {
        return iterator(head_->next, this);
    }
    
    const_iterator cbegin() const {
        return const_iterator(head_->next, this);
    }
 
    iterator end() {
        return iterator(tail_, this);
    }
    
    const_iterator cend() const {
        return const_iterator(tail_, this);
    }
 
    bool empty() const {
        return size_ == 0;
    }
 
    size_t size() const {
        return size_;
    }
 
    void clear() {
        Node* node = head_->next;
        while (node != tail_) {
            Node* next = node->next;
            delete node;
            node = next;
        }
        head_->next = tail_;
        tail_->prev = head_;
        
        for (size_t i = 0; i < bucket_count_; ++i) {
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }
 
    pair<iterator, bool> insert(const value_type& value) {
        Node* existing = find_node(value.first);
        if (existing != nullptr) {
            return pair<iterator, bool>(iterator(existing, this), false);
        }
        
        if (size_ >= static_cast<size_t>(bucket_count_ * LOAD_FACTOR)) {
            rehash(bucket_count_ * 2);
        }
        
        Node* new_node = new Node(value);
        
        size_t idx = get_bucket_index(value.first);
        new_node->hash_next = buckets_[idx];
        buckets_[idx] = new_node;
        
        new_node->prev = tail_->prev;
        new_node->next = tail_;
        tail_->prev->next = new_node;
        tail_->prev = new_node;
        
        ++size_;
        return pair<iterator, bool>(iterator(new_node, this), true);
    }
 
    void erase(iterator pos) {
        if (pos.get_node() == nullptr || pos.get_node() == tail_ || pos.get_map() != this) {
            throw invalid_iterator();
        }
        unlink_node(pos.get_node());
    }
 
    size_t count(const Key& key) const {
        return find_node(key) != nullptr ? 1 : 0;
    }
 
    iterator find(const Key& key) {
        Node* node = find_node(key);
        if (node == nullptr) {
            return end();
        }
        return iterator(node, this);
    }
    
    const_iterator find(const Key& key) const {
        Node* node = find_node(key);
        if (node == nullptr) {
            return cend();
        }
        return const_iterator(node, this);
    }
};

}

#endif
