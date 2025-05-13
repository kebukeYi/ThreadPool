#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include "vector"
#include "queue"
#include "memory"
#include "atomic"
#include "mutex"
#include "condition_variable"
#include "functional"
#include "unordered_map"
#include "Any.h"
#include "Semaphore.h"

enum class PoolMODE {
    MODE_FIXED, MODE_CACHE
};

class Task;

class Result {
public:
    Result(std::shared_ptr<Task> task, bool isValid = true);

    ~Result() = default;

    void setVal(Any any);

    Any get();

private:
    Any any_;
    Semaphore semaphore;
    std::shared_ptr<Task> task_;
    std::atomic_bool isValid_;
};

class Task {
    // 继承多态: 基类指针指向各种派生类对象;
public:
    Task();

    ~Task() = default;

    virtual Any run() = 0;

    void exec();

    void setResult(Result *result);

private:
    Result *result_;
};

class Thread {
public:
    // 线程函数变量 类型;
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc threadFunc);

    ~Thread();

    void start();

    int getId() const;

private:
    ThreadFunc threadFunc_;
    int threadId_;
    static int threadIdGen_;
};

class ThreadPool {
public:
    ThreadPool();

    ~ThreadPool();

    void setMode(PoolMODE poolMode);

    void setTaskQueThreshHold(size_t threshold);

    Result submitTask(const std::shared_ptr<Task> &task);

    void start(int initThreadPoolSize = std::thread::hardware_concurrency());

    // 不允许 线程池实例进行 拷贝构造和直接赋值操作;
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;

private:
    void threadFunc(int threadId);

private:
    // 根据 CPU 数量定义;
    size_t initThreadPoolSize_; // fixed 模式下 固定线程数量; cache模式下 最低线程数量;
    std::atomic_uint currentThreadSize_; // cache模式下, 当前线程数量;
    std::atomic_uint idleThreadSize_; // cache模式下, 空闲线程数量;
    int maxThreadSize_; // cache模式下,允许存在的最大线程数量;

    // std::vector<Thread *> threadPool_;
//    std::vector<std::unique_ptr<Thread>> threadPool_;
    std::unordered_map<int, std::unique_ptr<Thread>> threadPool_;

    // 直接使用 Task* ,可能会出现Task*提前析构(临时对象),导致后续无法正常执行;
    // std::queue<Task*> taskQue_;
    // 使用智能指针,既保证了生命周期延长,执行后又能自动释放资源;
    std::queue<std::shared_ptr<Task>> taskQue_;

    std::atomic_uint taskQueSize_;// 多线程读写 实时任务数量;
    int taskQueMaxThreshold_; // 最大任务数量;

    std::atomic_bool isRunning_;

    std::mutex taskQueMux_;

    // 池化一般两个基础点: 非满代表可生产, 非空代表可消费;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;

    std::condition_variable closePool_;

    PoolMODE poolMode_;
};


#endif //THREADPOOL_H
