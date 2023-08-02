#include "web.h"

extern void addfd( int epollfd, int fd, bool one_shot );
extern void removefd( int epollfd, int fd );

Web::Web()
{
    wb_users = new http_conn[MAX_FD];      //http请求队列
}

Web::~Web(){
    close(wb_epollfd);
    close(wb_listenfd);
    delete[] wb_users;
    delete wb_pool;
}

void Web::init(int port, int trigmode, int threadnum, int actormodel, int logclose, int logwrite){
    wb_port = port;
    wb_threadnum = threadnum;
    wb_trigmode = trigmode;
    wb_actormodel = actormodel;
    wb_lgclose = logclose;
    wb_lgwrite = logwrite;
}

void Web::threadPool(){
    wb_pool = new threadpool<http_conn>(wb_threadnum, MAX_EVENT_NUMBER);
}

void Web::log(){
    if (0 == wb_lgclose){
        if (1 == wb_lgwrite)
            Log::get_instance()->init("./ServerLog", wb_lgclose, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", wb_lgclose, 2000, 800000, 0);
    }
}

void Web::trigMode(){
    if (0 == wb_trigmode){
        wb_listenmode = 0;
        wb_connmode = 0;
    }else if (1 == wb_trigmode){
        wb_listenmode = 0;
        wb_connmode = 1;
    }else if (2 == wb_trigmode){
        wb_listenmode = 1;
        wb_connmode = 0;
    }else if (3 == wb_trigmode){
        wb_listenmode = 1;
        wb_connmode = 1;
    }
}

void Web::eventListen()
{
    wb_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(wb_port);

    int flag = 1, ret = 0;
    setsockopt(wb_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(wb_listenfd, (struct sockaddr *)&address, sizeof(address));
    ret = listen(wb_listenfd, 5);

    epoll_event events[MAX_EVENT_NUMBER];
    wb_epollfd = epoll_create(5);
    addfd(wb_epollfd, wb_listenfd, false);
    http_conn::conn_epollfd = wb_epollfd;
}

void Web::eventLoop()
{
    while (true){
        int number = epoll_wait(wb_epollfd, wb_events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR){
            printf("%s\n", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++){
            int sockfd = wb_events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == wb_listenfd){
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( wb_listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                
                if ( connfd < 0 ) {
                    printf( "errno is: %d\n", errno );
                    continue;
                } 

                if( http_conn::conn_user_count >= MAX_FD ) {
                    close(connfd);
                    continue;
                }
                wb_users[connfd].init( connfd, client_address, this->wb_lgclose);
            }
            else if (wb_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                wb_users[sockfd].close_conn();
            }
            //处理客户连接上接收到的数据
            else if (wb_events[i].events & EPOLLIN){
                if(wb_users[sockfd].read()) {
                    wb_pool->append(wb_users + sockfd);
                } else {
                    wb_users[sockfd].close_conn();
                }
            }
            else if (wb_events[i].events & EPOLLOUT){
                if( !wb_users[sockfd].write() ) {
                    wb_users[sockfd].close_conn();
                }
            }
        }
    }
}