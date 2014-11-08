#ifndef PRIORITY_SCHEDULER_H
#define PRIORITY_SCHEDULER_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

 
template <typename T>
class PriorityScheduler {
    private:
        priority_queue<T, vector<T>, greater<T> > q;
        mutex mtx;
        condition_variable cond;

    public: 
        void pop(T& item) {
            unique_lock<mutex> pop_lock(mtx);
            while (q.empty()) {
                cond.wait(pop_lock);
            }
            item = q.top();
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
