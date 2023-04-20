#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
//线程池类，定义为模板类是为了代码复用，模板参数T是任务类
template<typename T>
class threadpool{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T* request);
private:
    int m_thread_number;        //线程数量
    pthread_t *m_threads;       //线程数组
    int m_max_requests;         //请求队列种最多允许的、等待处理的请求数
    std::list<T*> m_workqueue;  //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    bool m_stop;                //是否结束线程

    /*工作线程运行的函数，不断从工作队列种取出任务并执行*/
    static void* worker(void* arg);
    void run();
};

#endif