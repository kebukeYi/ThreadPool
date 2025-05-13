#include "thread_pool.h"

#include "thread"
#include "functional"
#include "iostream"

const int TASK_MAX_SIZE = 80;
const int THREAD_MAX_SIZE = 8;
const int THREAD_MAX_TIME_OUT = 10; // 单位秒

ThreadPool::ThreadPool()
        : initThreadPoolSize_(0),
          currentThreadSize_(0),
          idleThreadSize_(0),
          taskQueSize_(0),
          maxThreadSize_(THREAD_MAX_SIZE),
          taskQueMaxThreshold_(TASK_MAX_SIZE),
          poolMode_(PoolMODE::MODE_FIXED) {}

ThreadPool::~ThreadPool() {
    isRunning_ = false;
//    notEmpty_.notify_all();// 唤醒所有线程进行判断是否释放资源;
    std::unique_lock<std::mutex> lock(taskQueMux_);
    notEmpty_.notify_all();// 唤醒所有线程进行判断是否释放资源;
    closePool_.wait(lock, [&]() -> bool {
        return threadPool_.size() == 0;
    });
}

void ThreadPool::setMode(PoolMODE poolMode) {
    poolMode_ = poolMode;
}

void ThreadPool::setTaskQueThreshHold(size_t threshold) {
    taskQueMaxThreshold_ = threshold;
}

// 生产任务;
Result ThreadPool::submitTask(const std::shared_ptr<Task> &task) {
    // 获得任务队列锁;
    std::unique_lock<std::mutex> lock(taskQueMux_);

    // 队列是否有空余的位置;
    // 方式1
//    while (taskQue_.size() == taskQueMaxThreshold_) {
//        notFull_.wait(lock);
//    }
    // 方式2
    // wait()一直等; wait_for();设置持续时间,比如10秒; wait_until();设置时间点,比如10点;
    // 用户阻塞时间不能最多阻塞1秒; 否则返回失败;
    if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]() -> bool {
        return taskQue_.size() < taskQueMaxThreshold_;
    })) {
        std::cout << "task queue is full, submit task fail;" << std::endl;
        return Result(task, false);
    }

    // 存在空余位置,添加;
    taskQue_.emplace(task);
    taskQueSize_++;

    // 队列有数据了, 通知 线程池的线程去消费;
    notEmpty_.notify_all();

    // 判断cache模式下, 是否需要创建新线程;
    if (poolMode_ == PoolMODE::MODE_CACHE &&
        currentThreadSize_ < maxThreadSize_ &&
        idleThreadSize_ < taskQueSize_) {
        // 创建线程;
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();
        // threadPool_.emplace_back(ptr); // unique 对象不允许普通拷贝和赋值;
        threadPool_.emplace(threadId, std::move(ptr)); // 因此进行资源转移;
        // 启动线程;
//        ptr->start();
        threadPool_[threadId]->start();
        std::cout << "thread new id :" << threadId << std::endl;
        // 修改当前线程数量;
        currentThreadSize_++;
    }

    // Result 是生命周期大于 task 的;
    return Result(task);
}

void ThreadPool::start(int initThreadPoolSize) {
    initThreadPoolSize_ = initThreadPoolSize;
    isRunning_ = true;
    for (int i = 0; i < initThreadPoolSize_; ++i) {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();
        // threadPool_.emplace_back(ptr); // unique 对象不允许普通拷贝和赋值;
        threadPool_.emplace(threadId, std::move(ptr)); // 因此进行资源转移;
        // ptr->start(); // std::move(ptr), 所以会导致null指针异常;
    }

    for (int i = 0; i < initThreadPoolSize_; ++i) {
        threadPool_[i]->start();
        currentThreadSize_++;
        idleThreadSize_++;
    }
}

// 线程池定义好 访问自己成员属性的函数,并让线程去绑定调用;
// 消费任务;
void ThreadPool::threadFunc(int threadId) {
//    std::cout << "threadFunc start, tid:" << std::this_thread::get_id() << std::endl;
//    std::cout << "threadFunc end, tid:" << std::this_thread::get_id() << std::endl;
    auto lastTime = std::chrono::high_resolution_clock::now();
    for (;;) {
        // 获得锁;
        std::unique_lock<std::mutex> lock(taskQueMux_);
        // 双重锁+判断;
        while (taskQue_.empty()) {
            if (!isRunning_) {
                threadPool_.erase(threadId);
                closePool_.notify_all();
                std::cout << "thread close id :" << threadId << std::endl;
                return;
            }
            if (poolMode_ == PoolMODE::MODE_CACHE) {
                auto res = notEmpty_.wait_for(lock, std::chrono::seconds(1));
                if (res == std::cv_status::timeout) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                    if (duration.count() > THREAD_MAX_TIME_OUT && currentThreadSize_ > initThreadPoolSize_) {
                        // 销毁线程;
                        threadPool_.erase(threadId);
                        // 数量减一;
                        currentThreadSize_--;
                        idleThreadSize_--;
                        std::cout << "thread exit id :" << threadId << std::endl;
                        return;
                    }
                }
            } else {
                // 检测是否非空,一直阻塞;
                notEmpty_.wait(lock);
            }
        }

        // 获取队列头部任务;
        auto task = taskQue_.front();
        taskQue_.pop();
        taskQueSize_--;
        idleThreadSize_--;

        // 如果还有任务, 可通知其他阻塞线程去消费;
        if (!taskQue_.empty()) {
            notEmpty_.notify_all();
        }

        // 取出任务后,进行通知,可生产;
        notFull_.notify_all();


        // 取出任务后,即可释放锁;
        lock.unlock();

        // 执行任务;
//        task->run();
        std::cout << "MyTask run, tid: " << threadId << std::endl;
        task->exec();
        idleThreadSize_++;
        std::cout << "MyTask end, tid: " << threadId << std::endl;
        lastTime = std::chrono::high_resolution_clock::now();
    }

}


int Thread::threadIdGen_ = 0;

Thread::Thread(ThreadFunc threadFunc) :
        threadFunc_(threadFunc),
        threadId_(threadIdGen_++) {
}

Thread::~Thread() {}

// 每个线程需要访问到 线程池中的任务队列,怎么访问?
void Thread::start() {
    // 启动一个线程, 执行线程池提供的执行函数;
    std::thread t(threadFunc_, threadId_);
    t.detach();// 分离t线程,防止当前函数 start()函数结束后,进行析构;
}

int Thread::getId() const {
    return threadId_;
}


Task::Task() : result_(nullptr) {
}

void Task::exec() {
    if (result_ != nullptr) {
        result_->setVal(run());
    }
}

void Task::setResult(Result *result) {
    result_ = result;
}

Result::Result(std::shared_ptr<Task> task, bool isValid) :
        task_(task), isValid_(isValid) {
    task_->setResult(this);
}

Any Result::get() {
    if (!isValid_) {
        return nullptr;
    }
    semaphore.wait();
    return std::move(any_);
}

void Result::setVal(Any any) {
    this->any_ = std::move(any);
    semaphore.post();
}