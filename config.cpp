#include "config.h"

Config::Config(){
    cfg_port = 9006;        //端口号,默认9006
    cfg_trigmode = 0;       //触发组合模式,默认listenfd LT + connfd LT
    cfg_threadnum = 8;      //线程池内的线程数量,默认8
    cfg_actormodel = 0;     //并发模型,默认是proactor
    cfg_lgclose = 0;        //日志开关,默认开启
    cfg_lgwrite = 0;        //日志写入方式，默认同步
}

void Config::parse_arg(int argc, char *argv[]){
    int opt;
    const char *str = "p:m:t:a:l:w:";
    while ((opt = getopt(argc, argv, str)) != -1){
        switch (opt){
            case 'p':{
                cfg_port = atoi(optarg);
                break;
            }
            case 'm':{
                cfg_trigmode = atoi(optarg);
                break;
            }
            case 't':{
                cfg_threadnum = atoi(optarg);
                break;
            }
            case 'a':{
                cfg_actormodel = atoi(optarg);
                break;
            }
            case 'l':{
                cfg_lgclose = atoi(optarg);
            }
            case 'w':{
                cfg_lgwrite = atoi(optarg);
            }
            default:
                break;
        }
    }
}