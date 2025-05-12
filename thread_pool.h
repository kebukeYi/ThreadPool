#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "vector"
#include "queue"
#include "memory"
#include "atomic"
#include "mutex"
#include "condition_variable"

enum class PoolMODE {
    MODE_FIXED, MODE_CACHE
};

class Task {
    // 多态: 基类指针指向各种派生类对象;
public:
    virtual void run() = 0;

private:
};

class Thread {
public:
    Thread();

    ~Thread();

    void start();

private:
};

class ThreadPool {
public:
    ThreadPool();

    ~ThreadPool();

    void setMode(PoolMODE poolMode);

    void setTaskQueThreshHold(size_t threshold);

    void submitTask(std::shared_ptr<Task> task);

    void start(int initThreadPoolSize = 4);


    // 不允许 线程池实例进行 拷贝构造和直接赋值操作;
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    void threadFunc();

private:
    // 可根据 CPU 数量定义;
    size_t initThreadPoolSize_;
    std::vector<Thread *> threadPool_;
    // 直接使用 Task* ,可能会出现Task*提前析构(临时对象),导致后续无法正常执行;
    // std::queue<Task*> taskQue_;
    // 使用智能指针,既保证了生命周期延长,执行后又能自动释放资源;
    std::queue<std::shared_ptr<Task>> taskQue_;
    std::atomic_uint taskQueSize_;// 多线程读写 实时任务数量;
    int taskQueMaxThreshold_; // 最大任务数量;

    std::mutex taskQueMux_;

    // 池化一般两个基础点: 非满代表可生产, 非空代表可消费;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;

    PoolMODE poolMode_;// 是否支持动态修改?
};


#endif //THREADPOOL_H
