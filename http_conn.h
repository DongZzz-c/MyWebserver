#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>
#include "log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;        // 文件名的最大长度
    static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;  // 写缓冲区的大小
    
    // HTTP请求方法，这里只支持GET
    enum METHOD {
        GET = 0, 
        POST, 
        HEAD, 
        PUT, 
        DELETE, 
        TRACE, 
        OPTIONS, 
        CONNECT
    };
    
    /*
        解析客户端请求时，主状态机的状态
        CHECK_STATE_REQUESTLINE:当前正在分析请求行
        CHECK_STATE_HEADER:当前正在分析头部字段
        CHECK_STATE_CONTENT:当前正在解析请求体
    */
    enum CHECK_STATE { 
        CHECK_STATE_REQUESTLINE = 0, 
        CHECK_STATE_HEADER, 
        CHECK_STATE_CONTENT 
    };
    
    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        BAD_REQUEST         :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
        FILE_REQUEST        :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */
    enum HTTP_CODE { 
        NO_REQUEST, 
        GET_REQUEST, 
        BAD_REQUEST, 
        NO_RESOURCE, 
        FORBIDDEN_REQUEST, 
        FILE_REQUEST, 
        INTERNAL_ERROR, 
        CLOSED_CONNECTION 
    };
    
    // 从状态机的三种可能状态，即行的读取状态，分别表示
    // 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整
    enum LINE_STATUS { 
        LINE_OK = 0, 
        LINE_BAD, 
        LINE_OPEN 
    };

public:
    http_conn(){}
    ~http_conn(){}

    void init(int sockfd, const sockaddr_in& addr, int lgclose);    // 初始化新接受的连接
    void close_conn();                                              // 关闭连接
    void process();                                                 // 处理客户端请求
    bool read();                                                    // 非阻塞读
    bool write();                                                   // 非阻塞写

private:
    void init();                                    // 初始化连接
    HTTP_CODE process_read();                       // 解析HTTP请求
    bool process_write( HTTP_CODE ret );            // 填充HTTP应答

    //被process_read调用以分析HTTP请求
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    LINE_STATUS parse_line();

    //被process_write调用以填充HTTP应答。
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_type();
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    static int conn_epollfd;       //所有socket上的事件都被注册到同一个epoll内核事件中
    static int conn_user_count;    //统计用户的数量

private:
    //基本
    sockaddr_in conn_address;                   //对方的socket地址
    int conn_sockfd;                            //该HTTP连接的socket
    int conn_content_length;                    //HTTP请求的消息总长度
    char conn_real_file[ FILENAME_LEN ];        //客户请求的目标文件的完整路径，内容等于doc_root + m_url, doc_root是网站根目录
    struct stat conn_file_stat;                 //目标文件的状态。判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
    char* conn_file_address;                    //客户请求的目标文件被mmap到内存中的起始位置
    int conn_lgclose;                           //日志开关

    //读
    CHECK_STATE conn_main_state;
    int conn_start_line;
    int conn_curpos_inline;                     //conn_checked_idx
    int conn_readpos_inbuf;                     //conn_read_idx
    char conn_read_buf[ READ_BUFFER_SIZE ];
    char* get_line() { return conn_read_buf + conn_start_line; }

    //解析请求行
    METHOD conn_method;                        //请求方法
    char* conn_url;                            //客户请求的目标文件的文件名
    char* conn_version;                        //HTTP协议版本号，我们仅支持HTTP1.1

    //解析请求头
    bool conn_linger;                          //HTTP请求是否要求保持连接
    char* conn_host;                           //主机名

    //写
    int conn_writepos_inbuf;                   //conn_write_idx
    char conn_write_buf[ WRITE_BUFFER_SIZE ];  //写缓冲区
    struct iovec conn_iv[2];                   //采用writev来执行写操作
    int conn_iv_count;                         //被写内存块的数量
    int conn_bytes_to_send;                    //将要发送的数据的字节数
    int conn_bytes_have_send;                  //已经发送的字节数
};

#endif
