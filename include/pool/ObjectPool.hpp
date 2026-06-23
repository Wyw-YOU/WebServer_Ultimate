#pragma once

#include <queue>
#include <mutex>
#include <cstddef>

template<typename T>
class ObjectPool
{
public:
    explicit ObjectPool(size_t maxSize = 1024)
        : maxSize_(maxSize) {}

    ~ObjectPool()
    {
        while(!pool_.empty())
        {
            delete pool_.front();
            pool_.pop();
        }
    }

    // 禁止拷贝
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    T* Get()
    {
        if(pool_.empty())
            return nullptr;
        T* obj = pool_.front();
        pool_.pop();
        return obj;
    }

    void Return(T* obj)
    {
        if(!obj)
            return;
        if(pool_.size() >= maxSize_)
        {
            delete obj;
            return;
        }
        pool_.push(obj);
    }

    size_t Size() const { return pool_.size(); }

private:
    std::queue<T*> pool_;
    size_t maxSize_;
};
