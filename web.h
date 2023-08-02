#ifndef WEB_H
#define WEB_H

#include "config.h"
#include "threadpool.h"
#include "http_conn.h"
#include "log.h"

const int MAX_FD = 65536;                //最大文件描述符
const int MAX_EVENT_NUMBER = 10000;      //最大事件数

class Web{
public:
    Web();
    ~Web();

    void init(int port, int trigmde, int threadnum, int actormode, int logclose, int logwrite);
    void threadPool();          //初始化线程池
    void log();                 //初始化日志
    void trigMode();            //选择触发模式
    void eventListen();         //事件监听
    void eventLoop();           //运行

    int wb_listenfd;            //监听
    int wb_epollfd;             //epoll

    http_conn *wb_users;                            //http请求队列
    threadpool<http_conn> *wb_pool;                 //线程池
    epoll_event wb_events[MAX_EVENT_NUMBER];        //epoll事件数组

    //传入参数
    int wb_threadnum;
    int wb_port;
    int wb_trigmode;
    int wb_listenmode;
    int wb_connmode;
    int wb_actormodel;
    int wb_lgwrite;
    int wb_lgclose;
};

#endif