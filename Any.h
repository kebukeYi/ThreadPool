//
// Created by 19327 on 2025/05/12/星期一.
//

#ifndef THREADPOOL_ANY_H
#define THREADPOOL_ANY_H


#include <memory>

class Any {
public:
    Any() = default;

    ~Any() = default;

    Any(const Any &) = delete;

    Any &operator=(const Any &) = delete;

    Any(Any &&) = default;

    Any &operator=(Any &&) = default;

    //用于接收任意数据
    template<typename T>
    Any(T data):base_(std::make_unique<Derived < T>>

    (data)) {}

    template<typename T>
    T cast_() {
        Derived <T> *pt = dynamic_cast<Derived <T> *>(base_.get());
        if (pt == nullptr) {
            throw "derive type is unmatch!";
        }
        return pt->data_;
    }

private:
    class Base { // 基类
    public:
        Base() = default;

        //保证派生类析构后，基类也执行析构
        virtual ~Base() = default;
    };

    template<typename T>
    class Derived : public Base { // 派生类
    public:
        T data_;

        Derived(T data) : data_(data) {}

        ~Derived() {}
    };

private:
    std::unique_ptr<Base> base_;
};


#endif //THREADPOOL_ANY_H
