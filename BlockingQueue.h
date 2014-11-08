#ifndef BLOCKING_QUEUE_H
#define BLOCK_QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

 
template <typename T>
class BlockingQueue {
    /* This class does not guarantee strong exception safety. */
    private:
        queue<T> q;
        mutex mtx;
        condition_variable cond;

    public: 
        T pop() {
            unique_lock<mutex> pop_lock(mtx); // automatically unlocks upon destruction
            while (q.empty()) {
                cond.wait(pop_lock);
            }
            T item = q.front();
            q.pop();

            return item;
        }

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
