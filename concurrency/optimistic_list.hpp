#pragma once

#include <tpcc/concurrency/backoff.hpp>
#include <tpcc/memory/bump_pointer_allocator.hpp>
#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/support/compiler.hpp>

#include <atomic>
#include <limits>
#include <mutex>
#include <thread>

namespace tpcc {
namespace solutions {

////////////////////////////////////////////////////////////////////////////////

// implement Test-and-TAS spinlock
class SpinLock {
 public:
  void Lock() {
    do {
      while (locked_.load()) {
        std::this_thread::yield();
      }
    } while (locked_.exchange(true));
  }

  void Unlock() {
    locked_.store(false);
  }

  // adapters for BasicLockable concept

  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }

 private:
  tpcc::atomic<bool> locked_{false};
};

////////////////////////////////////////////////////////////////////////////////

// don't touch this
template <typename T>
struct KeyTraits {
  static T LowerBound() {
    return std::numeric_limits<T>::min();
  }

  static T UpperBound() {
    return std::numeric_limits<T>::max();
  }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, class TTraits = KeyTraits<T>>
class OptimisticLinkedSet {
 private:
  struct Node {
    T key_;
    std::atomic<Node*> next_;
    SpinLock spinlock_;
    std::atomic<bool> marked_{false};

    Node(const T& key, Node* next = nullptr) : key_(key), next_(next) {
    }

    // use: auto node_lock = node->Lock();
    std::unique_lock<SpinLock> Lock() {
      return std::unique_lock<SpinLock>(spinlock_);
    }
  };

  struct EdgeCandidate {
    Node* pred_;
    Node* curr_;

    EdgeCandidate(Node* pred, Node* curr) : pred_(pred), curr_(curr) {
    }
  };

 public:
  explicit OptimisticLinkedSet(BumpPointerAllocator& allocator)
      : allocator_(allocator) {
    CreateEmptyList();
  }

  bool Insert(T key) {
    while (true) {
      EdgeCandidate current_edge = Locate(key);
      auto curr_temporary_lock = current_edge.curr_->Lock();
      auto pred_temporary_lock = current_edge.pred_->Lock();
      if (Validate(current_edge)) {
        if (current_edge.curr_->key_ != key) {
          Node* new_node = allocator_.New<Node>(key, current_edge.curr_);
          current_edge.pred_->next_.store(new_node);
          ++size_;
          return true;
        }
        return false;
      }
    }
  }

  bool Remove(const T& key) {
    while (true) {
      EdgeCandidate current_edge = Locate(key);
      auto curr_temporary_lock = current_edge.curr_->Lock();
      auto pred_temporary_lock = current_edge.pred_->Lock();
      if (Validate(current_edge)) {
        if (current_edge.curr_->key_ == key) {
          current_edge.curr_->marked_.store(true);
          current_edge.pred_->next_.store(current_edge.curr_->next_);
          --size_;
          return true;
        }
        return false;
      }
    }
  }

  bool Contains(const T& key) const {
    EdgeCandidate located_edge = Locate(key);
    return (located_edge.curr_->key_ == key);
  }

  size_t GetSize() const {
    return size_.load();
  }

 private:
  void CreateEmptyList() {
    head_ = allocator_.New<Node>(TTraits::LowerBound());
    head_->next_ = allocator_.New<Node>(TTraits::UpperBound());
  }

  EdgeCandidate Locate(const T& key) const {
    EdgeCandidate edge{head_, head_->next_.load()};
    while (edge.curr_->key_ < key) {
      edge.pred_ = edge.curr_;
      edge.curr_ = edge.curr_->next_.load();
    }
    return edge;
  }

  bool Validate(const EdgeCandidate& edge) const {
    return (!edge.pred_->marked_.load() && !edge.curr_->marked_.load() &&
            edge.pred_->next_.load() == edge.curr_);
  }

 private:
  BumpPointerAllocator& allocator_;
  Node* head_{nullptr};
  std::atomic<size_t> size_{0};
};

}  // namespace solutions
}  // namespace tpcc
