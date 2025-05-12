//
// Created by 19327 on 2025/05/12/星期一.
//
#include <iostream>
#include "thread_pool.h"
#include "chrono"
#include "thread"
#include "iostream"
#include "Any.h"

using ULong = unsigned long long;

class MyTask : public Task {
public:
    MyTask(int begin = 0, int end = 0) : begin_(begin), end_(end) {}

    Any run() override {
        std::cout << "MyTask run, tid: " << std::this_thread::get_id() << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds(3));
        ULong num = 0;
        for (ULong i = begin_; i <= end_; ++i) {
            num += i;
        }
        std::cout << "MyTask end; tid: " << std::this_thread::get_id() << std::endl;
        return num;
    }

private:
    int begin_;
    int end_;
};


int main() {
    ThreadPool pool;
    pool.start(4);

    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

    ULong sum1 = res1.get().cast_<ULong>();
    ULong sum2 = res2.get().cast_<ULong>();
    ULong sum3 = res3.get().cast_<ULong>();

    std::cout << (sum1 + sum2 + sum3) << std::endl;

    ULong num = 0;
    for (ULong i = 1; i <= 300000000; ++i) {
        num += i;
    }
    std::cout << (num) << std::endl;

    getchar();
}

