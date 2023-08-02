#include "config.h"
#include "web.h"

int main(int argc, char *argv[]){
    Config config;
    config.parse_arg(argc, argv);

    Web server;
    server.init(config.cfg_port, config.cfg_trigmode, config.cfg_threadnum, config.cfg_actormodel, config.cfg_lgclose, config.cfg_lgwrite);
    server.threadPool();        //创建线程池
    server.log();               //初始化日志
    server.trigMode();          //选择触发模式
    server.eventListen();       //事件监听
    server.eventLoop();         //运行
    return 0;
}