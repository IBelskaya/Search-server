#pragma once
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket{
       std::map<Key,Value> map; 
       std::mutex mutex_;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
           
        Access(const Key& key, Bucket& bucket): guard(bucket.mutex_), ref_to_value(bucket.map[key]){
        }
    };
     
    explicit ConcurrentMap(size_t bucket_count): buckets_(bucket_count){
    }
    
    Access operator[](const Key& key){
    auto& bucket=buckets_[static_cast<uint64_t>(key) % buckets_.size()];
    return {key, bucket};
    }
    
    size_t Erase(const Key& key) {
     uint64_t tmp_key = static_cast<uint64_t>(key) % buckets_.size();
    std::lock_guard guard(buckets_[tmp_key].mutex_);
    return buckets_[tmp_key].map.erase(key);
    }
    
    std::map<Key, Value> BuildOrdinaryMap(){
    std::map<Key, Value> result;
        for(auto& [map, mutex]: buckets_){
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets_;
};
