#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "locker.h"
using namespace std;

template <class T>
class block_queue{
public:
    block_queue(int max_size = 1000){
        if (max_size <= 0){
            exit(-1);
        }

        blkq_max_size = max_size;
        blkq_array = new T[max_size];
        blkq_size = 0;
        blkq_front = -1;
        blkq_back = -1;
    }

    void clear(){
        blkq_mutex.lock();
        blkq_size = 0;
        blkq_front = -1;
        blkq_back = -1;
        blkq_mutex.unlock();
    }

    ~block_queue(){
        blkq_mutex.lock();
        if (blkq_array != NULL)
            delete [] blkq_array;

        blkq_mutex.unlock();
    }

    //判断队列是否满了
    bool full() {
        blkq_mutex.lock();
        if (blkq_size >= blkq_max_size){
            blkq_mutex.unlock();
            return true;
        }
        blkq_mutex.unlock();
        return false;
    }

    //判断队列是否为空
    bool empty() {
        blkq_mutex.lock();
        if (0 == blkq_size){
            blkq_mutex.unlock();
            return true;
        }
        blkq_mutex.unlock();
        return false;
    }

    //返回队首元素
    bool front(T &value) {
        blkq_mutex.lock();
        if (0 == blkq_size){
            blkq_mutex.unlock();
            return false;
        }
        value = blkq_array[blkq_front];
        blkq_mutex.unlock();
        return true;
    }

    //返回队尾元素
    bool back(T &value) {
        blkq_mutex.lock();
        if (0 == blkq_size){
            blkq_mutex.unlock();
            return false;
        }
        value = blkq_array[blkq_back];
        blkq_mutex.unlock();
        return true;
    }

    int size() {
        int tmp = 0;
        blkq_mutex.lock();
        tmp = blkq_size;
        blkq_mutex.unlock();
        return tmp;
    }

    int max_size(){
        int tmp = 0;
        blkq_mutex.lock();
        tmp = blkq_max_size;
        blkq_mutex.unlock();
        return tmp;
    }

    //往队列添加元素，需要将所有使用队列的线程先唤醒
    //当有元素push进队列,相当于生产者生产了一个元素
    //若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item){

        blkq_mutex.lock();
        if (blkq_size >= blkq_max_size){
            blkq_cond.broadcast();
            blkq_mutex.unlock();
            return false;
        }

        blkq_back = (blkq_back + 1) % blkq_max_size;
        blkq_array[blkq_back] = item;

        blkq_size++;

        blkq_cond.broadcast();
        blkq_mutex.unlock();
        return true;
    }
    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item){

        blkq_mutex.lock();
        while (blkq_size <= 0){
            if (!blkq_cond.wait(blkq_mutex.get())){
                blkq_mutex.unlock();
                return false;
            }
        }

        blkq_front = (blkq_front + 1) % blkq_max_size;
        item = blkq_array[blkq_front];
        blkq_size--;
        blkq_mutex.unlock();
        return true;
    }

    //增加了超时处理
    bool pop(T &item, int ms_timeout){
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        blkq_mutex.lock();
        if (blkq_size <= 0){
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!blkq_cond.timewait(blkq_mutex.get(), t)){
                blkq_mutex.unlock();
                return false;
            }
        }

        if (blkq_size <= 0){
            blkq_mutex.unlock();
            return false;
        }

        blkq_front = (blkq_front + 1) % blkq_max_size;
        item = blkq_array[blkq_front];
        blkq_size--;
        blkq_mutex.unlock();
        return true;
    }

private:
    locker blkq_mutex;
    cond blkq_cond;
    T *blkq_array;
    int blkq_size;
    int blkq_max_size;
    int blkq_front;
    int blkq_back;
};

#endif
