#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log(){
    lg_count = 0;
    lg_is_async = false;
}

Log::~Log(){
    if (lg_fp != NULL){
        fclose(lg_fp);
    }
}

//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size){
    //如果设置了max_queue_size,则设置为异步
    if (max_queue_size >= 1){
        lg_is_async = true;
        lg_blkq = new block_queue<string>(max_queue_size);
        pthread_t tid;
        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
    
    lg_close = close_log;
    lg_buf_size = log_buf_size;
    lg_buf = new char[lg_buf_size];
    memset(lg_buf, '\0', lg_buf_size);
    lg_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
 
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (p == NULL){
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }else{
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    lg_today = my_tm.tm_mday;
    
    lg_fp = fopen(log_full_name, "a");
    if (lg_fp == NULL){
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...){
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level){
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    lg_mutex.lock();
    lg_count++;

    //everyday log
    if (lg_today != my_tm.tm_mday || lg_count % lg_split_lines == 0){
        char new_log[256] = {0};
        fflush(lg_fp);
        fclose(lg_fp);
        char tail[16] = {0};
       
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       
        if (lg_today != my_tm.tm_mday){
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            lg_today = my_tm.tm_mday;
            lg_count = 0;
        }else{
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, lg_count / lg_split_lines);
        }
        lg_fp = fopen(new_log, "a");
    }
 
    lg_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    string log_str;
    lg_mutex.lock();

    //写入的具体时间内容格式
    int n = snprintf(lg_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    int m = vsnprintf(lg_buf + n, lg_buf_size - n - 1, format, valst);
    lg_buf[n + m] = '\n';
    lg_buf[n + m + 1] = '\0';
    log_str = lg_buf;

    lg_mutex.unlock();

    if (lg_is_async && !lg_blkq->full()){
        lg_blkq->push(log_str);
    }else{
        lg_mutex.lock();
        fputs(log_str.c_str(), lg_fp);
        lg_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void){
    lg_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(lg_fp);
    lg_mutex.unlock();
}
