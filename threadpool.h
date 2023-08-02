#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"

template<typename T>
class threadpool {
public:
    threadpool(int threadnumber , int requests);
    ~threadpool();
    bool append(T* request);

private:
    static void* worker(void* arg);
    void run();

private:
    int pool_threadnum;
    int pool_requests;
    bool pool_stop;
    pthread_t *pool_threads;         //线程数组
    std::list<T*> pool_workqueue;    //请求队列
    sem pool_queuestate;             //是否有任务需要处理
    locker pool_lock;          
};

template< typename T >
threadpool< T >::threadpool(int threadnumber, int requests) : 
        pool_threadnum(threadnumber), pool_requests(requests), 
        pool_stop(false), pool_threads(NULL) {

    if((threadnumber <= 0) || (requests <= 0) ) {
        throw std::exception();
    }

    pool_threads = new pthread_t[pool_threadnum];
    if(!pool_threads) {
        throw std::exception();
    }

    for ( int i = 0; i < pool_threadnum; ++i ) {
        if(pthread_create(pool_threads + i, NULL, worker, this ) != 0) {
            delete [] pool_threads;
            throw std::exception();
        }
        
        if( pthread_detach( pool_threads[i] ) ) {
            delete [] pool_threads;
            throw std::exception();
        }
    }
}

template< typename T >
threadpool< T >::~threadpool() {
    delete [] pool_threads;
    pool_stop = true;
}

template< typename T >
bool threadpool< T >::append( T* request )
{
    pool_lock.lock();
    if ( pool_workqueue.size() > pool_requests ) {
        pool_lock.unlock();
        return false;
    }
    pool_workqueue.push_back(request);
    pool_lock.unlock();
    pool_queuestate.post();
    return true;
}

template< typename T >
void* threadpool< T >::worker( void* arg )
{
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run() {

    while (!pool_stop) {
        pool_queuestate.wait();
        pool_lock.lock();
        if ( pool_workqueue.empty() ) {
            pool_lock.unlock();
            continue;
        }
        T* request = pool_workqueue.front();
        pool_workqueue.pop_front();
        pool_lock.unlock();
        if ( !request ) {
            continue;
        }
        request->process();
    }

}

#endif
