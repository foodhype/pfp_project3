#ifndef LIFO_SCHEDULER_H
#define LIFO_SCHEDLER_H

#include <stack>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

 
template <typename T>
class LIFOScheduler {
    private:
        stack<T> s;
        mutex mtx;
        condition_variable cond;

    public: 
        void pop(T& item) {
            unique_lock<mutex> pop_lock(mtx);
            while (s.empty()) {
                cond.wait(pop_lock);
            }
            item = s.top();
            s.pop();
        }
        
        void push(const T& item) {
            unique_lock<mutex> push_lock(mtx);
            s.push(item);
            push_lock.unlock();
            cond.notify_one();
        } 

        bool empty() {
            return s.empty();
        }
};

#endif
