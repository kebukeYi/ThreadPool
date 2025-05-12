#include "thread_pool.h"
#include "functional"

const int TASK_MAX_SIZE = 1024;

ThreadPool::ThreadPool()
        : initThreadPoolSize_(4),
          taskQueSize_(0),
          taskQueMaxThreshold_(TASK_MAX_SIZE),
          poolMode_(PoolMODE::MODE_FIXED) {}

ThreadPool::~ThreadPool() {}

void ThreadPool::setMode(PoolMODE poolMode) {
    poolMode_ = poolMode;
}

void ThreadPool::setTaskQueThreshHold(size_t threshold) {
    taskQueMaxThreshold_ = threshold;
}

void ThreadPool::submitTask(std::shared_ptr<Task> task) {

}

void ThreadPool::start(int initThreadPoolSize) {
    initThreadPoolSize_ = initThreadPoolSize;

    for (int i = 0; i < initThreadPoolSize_; ++i) {
        threadPool_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }

    for (int i = 0; i < initThreadPoolSize_; ++i) {
        threadPool_[i]->start();
    }
}

// 线程池定义好 访问自己成员属性的函数,并让线程去绑定调用;
void ThreadPool::threadFunc() {

}

// 每个线程需要访问到 线程池中的任务队列,怎么访问?
void Thread::start() {
    // 执行线程池提供的任函数;
}
