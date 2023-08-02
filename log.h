#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance(){
        static Log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args){
        Log::get_instance()->async_write_log();
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    void write_log(int level, const char *format, ...);
    void flush(void);

private:
    Log();
    virtual ~Log();
    void *async_write_log(){
        string single_log;
        //从阻塞队列中取出一个日志string，写入文件
        while (lg_blkq->pop(single_log)){
            lg_mutex.lock();
            fputs(single_log.c_str(), lg_fp);
            lg_mutex.unlock();
        }
    }

private:
    char dir_name[128];             //路径名
    char log_name[128];             //log文件名

    long long lg_count;             //日志行数记录
    bool lg_is_async;               //是否同步标志位
    block_queue<string> *lg_blkq;   //阻塞队列
    int lg_close;                   //关闭日志
    int lg_split_lines;             //日志最大行数
    int lg_buf_size;                //日志缓冲区大小
    char *lg_buf;                   //日志缓冲区

    int lg_today;                   //因为按天分类,记录当前时间是那一天
    FILE *lg_fp;                    //打开log的文件指针

    locker lg_mutex;

};

//用于不同类型的日志输出
#define LOG_DEBUG(format, ...) if(0 == conn_lgclose) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == conn_lgclose) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == conn_lgclose) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == conn_lgclose) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

#endif
