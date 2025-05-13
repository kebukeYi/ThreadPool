//
// Created by 19327 on 2025/05/12/星期一.
//

#ifndef THREADPOOL_SEMAPHORE_H
#define THREADPOOL_SEMAPHORE_H

#include "mutex"
#include "condition_variable"

class Semaphore {
public:
    Semaphore(int limitSize = 0) : limitSize_(limitSize), isExist_(true) {}

    ~Semaphore() {
        isExist_ = false;
    }

public:
    void wait() {
        if (!isExist_)return;
        std::unique_lock<std::mutex> lock(mutex_);
        con_.wait(lock, [&]() -> bool {
            return limitSize_ > 0;
        });
        limitSize_--;
    }

    void post() {
        if (!isExist_)return;
        std::unique_lock<std::mutex> lock(mutex_);
        limitSize_++;
        con_.notify_all();
    }

private:
    std::atomic<bool> isExist_;
    std::mutex mutex_;
    std::condition_variable con_;
    int limitSize_;
};

#endif //THREADPOOL_SEMAPHORE_H
