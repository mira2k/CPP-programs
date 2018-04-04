#pragma once

#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/stdlike/condition_variable.hpp>
#include <tpcc/stdlike/mutex.hpp>

#include <tpcc/support/compiler.hpp>

#include <atomic>
#include <algorithm>
#include <iostream>
#include <forward_list>
#include <functional>
#include <shared_mutex>
#include <vector>
#include <utility>

namespace tpcc {
    namespace solutions {

////////////////////////////////////////////////////////////////////////////////

// implement writer-priority rwlock
        class ReaderWriterLock {
        public:
            // reader section / shared ownership

            void lock_shared() {
                std::unique_lock<tpcc::mutex> temporary_lock(mutex_);
                while (writer_) {
                    readers_block_.wait(temporary_lock);
                }
                ++read_acquires_;
            }

            void unlock_shared() {
                std::unique_lock<tpcc::mutex> temporary_lock(mutex_);
                --read_acquires_;
                if (read_acquires_ == 0) {
                    writer_block_.notify_all();
                }
            }

            // writer section / exclusive ownership

            void lock() {
                std::unique_lock<tpcc::mutex> temporary_lock(mutex_);
                ++write_acquires_;
                while (writer_) {
                    writer_block_.wait(temporary_lock);
                }
                writer_ = true;
                while (read_acquires_ != 0) {
                    writer_block_.wait(temporary_lock);
                }
            }

            void unlock() {
                std::unique_lock<tpcc::mutex> temporary_lock(mutex_);
                writer_ = false;
                readers_block_.notify_all();
                writer_block_.notify_all();
            }

        private:
            // use mutex / condition_variable from tpcc namespace
            bool writer_{false};
            size_t read_acquires_{0};
            size_t write_acquires_{0};
            tpcc::condition_variable writer_block_;
            tpcc::condition_variable readers_block_;
            tpcc::mutex mutex_;
        };

////////////////////////////////////////////////////////////////////////////////

        template <typename T, class HashFunction = std::hash<T>>
        class StripedHashSet {
        private:
            using RWLock = ReaderWriterLock;  // std::shared_timed_mutex

            using ReaderLocker = std::shared_lock<RWLock>;
            using WriterLocker = std::unique_lock<RWLock>;

            using Bucket = std::forward_list<T>;
            using Buckets = std::vector<Bucket>;

        public:
            explicit StripedHashSet(const size_t concurrency_level = 4,
                                    const size_t growth_factor = 2,
                                    const double max_load_factor = 0.8)
                    : concurrency_level_(concurrency_level),
                      growth_factor_(growth_factor),
                      max_load_factor_(max_load_factor),
                      stripes_(concurrency_level),
                      buckets_(concurrency_level * growth_factor) {
            }

            bool Insert(T element) {
                std::size_t hash_value = hash_(element);
                auto temporary_lock = LockStripe<WriterLocker>(hash_value);

                auto& bucket = GetBucket(hash_value);
                auto result = std::find(bucket.begin(), bucket.end(), element);

                if (result == bucket.end()) {
                    bucket.emplace_front(element);
                    set_size_++;
                    if (MaxLoadFactorExceeded()) {
                        std::size_t exp_cnt = growth_factor_ * buckets_.size();
                        temporary_lock.unlock();
                        TryExpandTable(exp_cnt);
                    }
                    return true;
                }
                return false;
            }

            bool Remove(const T& element) {
                const std::size_t hash_value = hash_(element);
                auto temporary_lock = LockStripe<WriterLocker>(hash_value);
                auto& bucket = GetBucket(hash_value);
                if (std::find(bucket.begin(), bucket.end(), element) != bucket.end()) {
                    bucket.remove(element);
                    set_size_.fetch_sub(1);
                    return true;
                }
                return false;
            }

            bool Contains(const T& element) const {
                std::size_t hash_value = hash_(element);
                auto temporary_lock = LockStripe<ReaderLocker>(hash_value);
                const auto& bucket = GetBucket(hash_value);
                return (std::find(bucket.begin(), bucket.end(), element) != bucket.end());
            }

            size_t GetSize() const {
                return set_size_.load();  // to be implemented
            }

            size_t GetBucketCount() const {
                WriterLocker temporary_lock(stripes_[0]);
                return buckets_.size();
            }

        private:
            size_t GetStripeIndex(const size_t hash_value) const {
                return hash_value % stripes_.size();
            }

            // use: auto stripe_lock = LockStripe<ReaderLocker>(hash_value);
            template <class Locker>
            Locker LockStripe(const size_t hash_value) const {
                const std::size_t stripe = GetStripeIndex(hash_value);
                return Locker(stripes_[GetStripeIndex(stripe)]);
            }

            size_t GetBucketIndex(const size_t hash_value) const {
                return hash_value % buckets_.size();
            }

            Bucket& GetBucket(const size_t hash_value) {
                return buckets_[GetBucketIndex(hash_value)];
            }

            const Bucket& GetBucket(const size_t hash_value) const {
                return buckets_[GetBucketIndex(hash_value)];
            }

            bool MaxLoadFactorExceeded() const {
                return (set_size_.load() >= max_load_factor_ * buckets_.size());
            }

            void TryExpandTable(const size_t expected_bucket_count) {
                std::vector<WriterLocker> temporary_locks;
                temporary_locks.emplace_back(stripes_[0]);
                if (MaxLoadFactorExceeded()) {
                    for (std::size_t i = 1; i < stripes_.size(); ++i) {
                        temporary_locks.emplace_back(stripes_[i]);
                    }
                    std::size_t new_bucket_count = buckets_.size() * growth_factor_;
                    Buckets new_buckets(new_bucket_count);
                    for (Bucket& bucket : buckets_) {
                        for (auto& item : bucket) {
                            std::size_t item_hash = hash_(item);
                            std::size_t new_bucket_index = item_hash % new_bucket_count;
                            new_buckets[new_bucket_index].push_front(std::move(item));
                        }
                    }
                    buckets_.swap(new_buckets);
                }
            }

        private:
            size_t concurrency_level_;
            std::atomic<size_t> set_size_{0};
            size_t growth_factor_;
            double max_load_factor_;
            HashFunction hash_;
            Buckets buckets_;
            mutable std::vector<RWLock> stripes_;
        };

    }  // namespace solutions
}  // namespace tpcc
