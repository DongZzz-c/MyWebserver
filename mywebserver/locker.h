#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <semaphore.h>
using namespace std;
//线程同步机制封装类，包括互斥锁，条件变量，信号量

//互斥锁
class locker {
public:
    locker() {
        if(pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw exception();
        }
    }
    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }
    //上锁
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    //解锁
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    //获取锁
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

//条件变量
class cond {
public:
    cond(){
        if (pthread_cond_init(&m_cond, NULL) != 0) {
            throw exception();
        }
    }
    ~cond() {
        pthread_cond_destroy(&m_cond);
    }
    //阻塞等待
    bool wait(pthread_mutex_t *m_mutex) {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, m_mutex);
        return ret == 0;
    }
    //阻塞等待固定时间
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t) {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        return ret == 0;
    }
    //唤醒一个或多个等待线程
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //唤醒所有等待线程
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;
};

class sem {
public:
    sem() {
        if( sem_init( &m_sem, 0, 0 ) != 0 ) {
            throw exception();
        }
    }
    sem(int num) {
        if( sem_init( &m_sem, 0, num ) != 0 ) {
            throw exception();
        }
    }
    ~sem() {
        sem_destroy( &m_sem );
    }
    //等待信号链
    bool wait() {
        return sem_wait( &m_sem ) == 0;
    }
    //增加信号量
    bool post() {
        return sem_post( &m_sem ) == 0;
    }
private:
    sem_t m_sem;
};
#endif