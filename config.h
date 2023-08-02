#ifndef CONFIG_H
#define CONFIG_H

#include <unistd.h>
#include <cstdlib>

using namespace std;

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char*argv[]);

    int cfg_port;           //端口号
    int cfg_trigmode;       //触发组合模式
    int cfg_threadnum;      //线程池内的线程数量
    int cfg_actormodel;     //并发模型选择
    int cfg_lgclose;        //开启日志
    int cfg_lgwrite;        //日志写入方式
};

#endif