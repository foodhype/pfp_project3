#ifndef FIFO_SCHEDULER_H
#define FIFO_SCHEDULER_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

 
template <typename T>
class FIFOScheduler {
    private:
        queue<T> q;
        mutex mtx;
        condition_variable cond;

    public: 
        void pop(T& item) {
            unique_lock<mutex> pop_lock(mtx);
            while (q.empty()) {
                cond.wait(pop_lock);
            }
            item = q.front();
            q.pop();
        }
        
        void push(const T& item) {
            unique_lock<mutex> push_lock(mtx);
            q.push(item);
            push_lock.unlock();
            cond.notify_one();
        } 

        bool empty() {
            return q.empty();
        }
};

#endif
