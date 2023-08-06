#include "http_conn.h"

//定义HTTP响应的状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";

// 网站根目录
const char* doc_root = "/home/dong/jobprog/mywebserver/MyWebserver/resources";

int http_conn::conn_user_count = 0;
int http_conn::conn_epollfd = -1;

int setnonblocking( int fd ) {
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

//向epoll中添加需要监听的文件描述符
void addfd( int epollfd, int fd, bool one_shot ) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if(one_shot) {
        //防止同一个通信被不同的线程处理
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    //设置文件描述符非阻塞
    setnonblocking(fd);  
}

//从epoll中移除监听的文件描述符
void removefd( int epollfd, int fd ) {
    epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
    close(fd);
}

//修改文件描述符，重置socket上的EPOLLONESHOT事件
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl( epollfd, EPOLL_CTL_MOD, fd, &event );
}

//解除内存映射
void http_conn::unmap() {
    if( conn_file_address )
    {
        munmap( conn_file_address, conn_file_stat.st_size );
        conn_file_address = 0;
    }
}

// 写HTTP响应
bool http_conn::write()
{
    int temp = 0;
    if ( conn_bytes_to_send == 0 ) {
        modfd( conn_epollfd, conn_sockfd, EPOLLIN ); 
        init();
        return true;
    }

    while(1) {
        //分散写
        temp = writev(conn_sockfd, conn_iv, conn_iv_count);
        if ( temp <= -1 ) {
            // 如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件，虽然在此期间，
            // 服务器无法立即接收到同一客户的下一个请求，但可以保证连接的完整性。
            if( errno == EAGAIN ) {
                modfd( conn_epollfd, conn_sockfd, EPOLLOUT );
                return true;
            }
            unmap();
            return false;
        }
        conn_bytes_have_send += temp;
        conn_bytes_to_send -= temp;

        if (conn_bytes_have_send >= conn_iv[0].iov_len){
            conn_iv[0].iov_len = 0;
            conn_iv[1].iov_base = conn_file_address + (conn_bytes_have_send - conn_writepos_inbuf);
            conn_iv[1].iov_len = conn_bytes_to_send;
        }else{
            conn_iv[0].iov_base = conn_write_buf + conn_bytes_have_send;
            conn_iv[0].iov_len = conn_iv[0].iov_len - temp;
        }

        if (conn_bytes_to_send <= 0){
            //没有数据要发送了
            unmap();
            modfd(conn_epollfd, conn_sockfd, EPOLLIN);

            if (conn_linger){
                init();
                return true;
            }else{
                return false;
            }
        }
    }
}

//关闭连接
void http_conn::close_conn() {
    if(conn_sockfd != -1) {
        removefd(conn_epollfd, conn_sockfd);
        conn_sockfd = -1;
        conn_user_count--; // 关闭一个连接，将客户总数量-1
    }
}

//循环读取客户数据，直到无数据可读或者对方关闭连接
bool http_conn::read() {
    if( conn_readpos_inbuf >= READ_BUFFER_SIZE ) {
        return false;
    }
    int bytes_read = 0;
    while(true) {
        // 从conn_read_buf + conn_readpos_inbuf索引出开始保存数据，大小是READ_BUFFER_SIZE - conn_readpos_inbuf
        bytes_read = recv(conn_sockfd, conn_read_buf + conn_readpos_inbuf, 
        READ_BUFFER_SIZE - conn_readpos_inbuf, 0 );
        if (bytes_read == -1) {
            if( errno == EAGAIN || errno == EWOULDBLOCK ) {
                break;
            }
            return false;   
        } else if (bytes_read == 0) {
            return false;
        }
        conn_readpos_inbuf += bytes_read;
    }
    return true;
}

void http_conn::init()
{
    conn_main_state = CHECK_STATE_REQUESTLINE;
    conn_linger = false;
    conn_method = GET;
    conn_url = 0;              
    conn_version = 0;
    conn_content_length = 0;
    conn_host = 0;
    conn_start_line = 0;
    conn_curpos_inline = 0;
    conn_readpos_inbuf = 0;
    conn_writepos_inbuf = 0;
    conn_bytes_to_send = 0;
    conn_bytes_have_send = 0;
    bzero(conn_read_buf, READ_BUFFER_SIZE);
    bzero(conn_write_buf, READ_BUFFER_SIZE);
    bzero(conn_real_file, FILENAME_LEN);
}

//初始化连接
void http_conn::init(int sockfd, const sockaddr_in& addr, int lgclose){
    conn_sockfd = sockfd;
    conn_address = addr;
    conn_lgclose = lgclose;
    
    // 端口复用
    int reuse = 1;
    setsockopt( conn_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
    addfd( conn_epollfd, sockfd, true );
    conn_user_count++;
    init();
}

bool http_conn::add_content( const char* content ){
    return add_response( "%s", content );
}

bool http_conn::add_blank_line(){
    return add_response( "%s", "\r\n" );
}

bool http_conn::add_linger(){
    return add_response( "Connection: %s\r\n", ( conn_linger == true ) ? "keep-alive" : "close" );
}

bool http_conn::add_content_type() {
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool http_conn::add_content_length(int content_len) {
    return add_response( "Content-Length: %d\r\n", content_len );
}

bool http_conn::add_headers(int content_len) {
    add_content_length(content_len);
    add_content_type();
    add_linger();
    add_blank_line();
}

bool http_conn::add_status_line( int status, const char* title ) {
    return add_response( "%s %d %s\r\n", "HTTP/1.1", status, title );
}

// 往写缓冲中写入待发送的数据
bool http_conn::add_response( const char* format, ... ) {
    if( conn_writepos_inbuf >= WRITE_BUFFER_SIZE ) {
        return false;
    }
    //使用可变参数列表处理格式化字符串
    va_list arg_list;
    va_start( arg_list, format );
    // 将格式化后的字符串写入到写缓冲区中
    int len = vsnprintf( 
        conn_write_buf + conn_writepos_inbuf, 
        WRITE_BUFFER_SIZE - 1 - conn_writepos_inbuf, 
        format,                 //格式化字符串
        arg_list                //可变参数列表
    );
    if( len >= ( WRITE_BUFFER_SIZE - 1 - conn_writepos_inbuf ) ) {
        return false;
    }
    conn_writepos_inbuf += len;
    va_end( arg_list );
    return true;
}

//根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret) {
    switch (ret)
    {
        case INTERNAL_ERROR:
            add_status_line( 500, error_500_title );
            add_headers( strlen( error_500_form ) );
            if ( ! add_content( error_500_form ) ) {
                return false;
            }
            break;
        case BAD_REQUEST:
            add_status_line( 400, error_400_title );
            add_headers( strlen( error_400_form ) );
            if ( ! add_content( error_400_form ) ) {
                return false;
            }
            break;
        case NO_RESOURCE:
            add_status_line( 404, error_404_title );
            add_headers( strlen( error_404_form ) );
            if ( ! add_content( error_404_form ) ) {
                return false;
            }
            break;
        case FORBIDDEN_REQUEST:
            add_status_line( 403, error_403_title );
            add_headers(strlen( error_403_form));
            if ( ! add_content( error_403_form ) ) {
                return false;
            }
            break;
        case FILE_REQUEST:
            add_status_line(200, ok_200_title );
            add_headers(conn_file_stat.st_size);

            conn_iv[ 0 ].iov_base = conn_write_buf;
            conn_iv[ 0 ].iov_len = conn_writepos_inbuf;
            conn_iv[ 1 ].iov_base = conn_file_address;
            conn_iv[ 1 ].iov_len = conn_file_stat.st_size;
            conn_iv_count = 2;

            conn_bytes_to_send = conn_writepos_inbuf + conn_file_stat.st_size;

            return true;
        default:
            return false;
    }

    conn_iv[ 0 ].iov_base = conn_write_buf;
    conn_iv[ 0 ].iov_len = conn_writepos_inbuf;
    conn_iv_count = 1;
    conn_bytes_to_send = conn_writepos_inbuf;
    return true;
}

//没有真正解析HTTP请求的消息体，只判断它是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content( char* text ) {
    if ( conn_readpos_inbuf >= ( conn_content_length + conn_curpos_inline ) )
    {
        text[ conn_content_length ] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

//当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性，
//如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其映射到内存地址m_file_address处，并告诉调用者获取文件成功
http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy( conn_real_file, doc_root );
    int len = strlen( doc_root );
    strncpy( conn_real_file + len, conn_url, FILENAME_LEN - len - 1 );
    //获取文件的相关的状态信息，放入conn_file_stat中
    if ( stat( conn_real_file, &conn_file_stat ) < 0 ) {
        return NO_RESOURCE;
    }
    //判断访问权限
    if ( ! ( conn_file_stat.st_mode & S_IROTH ) ) {
        return FORBIDDEN_REQUEST;
    }
    //判断是否是目录
    if ( S_ISDIR( conn_file_stat.st_mode ) ) {
        return BAD_REQUEST;
    }
    //以只读方式打开文件
    int fd = open( conn_real_file, O_RDONLY );
    //创建内存映射
    conn_file_address = ( char* )mmap( 0, conn_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close( fd );
    return FILE_REQUEST;
}

//解析请求头
http_conn::HTTP_CODE http_conn::parse_headers(char* text) {   
    // 遇到空行，表示头部字段解析完毕
    if( text[0] == '\0' ) {
        if ( conn_content_length != 0 ) {
            conn_main_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }else if ( strncasecmp( text, "Connection:", 11 ) == 0 ) {
        // 处理Connection 头部字段  Connection: keep-alive
        text += 11;
        text += strspn( text, " \t" );
        if ( strcasecmp( text, "keep-alive" ) == 0 ) {
            conn_linger = true;
        }
    }else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 ) {
        // 处理Content-Length头部字段
        text += 15;
        text += strspn( text, " \t" );
        conn_content_length = atol(text);
    }else if ( strncasecmp( text, "Host:", 5 ) == 0 ) {
        // 处理Host头部字段
        text += 5;
        text += strspn( text, " \t" );
        conn_host = text;
    } else {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

//解析请求行
http_conn::HTTP_CODE http_conn::parse_request_line(char* text) {
    // GET /index.html HTTP/1.1
    conn_url = strpbrk(text, " \t");
    if (! conn_url) { 
        return BAD_REQUEST;
    }
    //请求方法
    *conn_url++ = '\0';
    char* method = text;
    if ( strcasecmp(method, "GET") == 0 ) {
        conn_method = GET;
    } else {
        return BAD_REQUEST;
    }
    //版本号
    conn_version = strpbrk( conn_url, " \t" );
    if (!conn_version) {
        return BAD_REQUEST;
    }
    *conn_version++ = '\0';
    if (strcasecmp( conn_version, "HTTP/1.1") != 0 ) {
        return BAD_REQUEST;
    }
    //Url http://192.168.110.129:10000/index.html
    if (strncasecmp(conn_url, "http://", 7) == 0 ) {   
        conn_url += 7;
        conn_url = strchr( conn_url, '/' );
    }
    if ( !conn_url || conn_url[0] != '/' ) {
        return BAD_REQUEST;
    }
    conn_main_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}


//读一行进readbuf，判断依据\r\n
http_conn::LINE_STATUS http_conn::parse_line() {
    char temp;
    for ( ; conn_curpos_inline < conn_readpos_inbuf; ++conn_curpos_inline ) {
        temp = conn_read_buf[ conn_curpos_inline ];
        if ( temp == '\r' ) {
            if ( ( conn_curpos_inline + 1 ) == conn_readpos_inbuf ) {
                return LINE_OPEN;
            } else if ( conn_read_buf[ conn_curpos_inline + 1 ] == '\n' ) {
                conn_read_buf[ conn_curpos_inline++ ] = '\0';
                conn_read_buf[ conn_curpos_inline++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if( temp == '\n' )  {
            if( ( conn_curpos_inline > 1) && ( conn_read_buf[ conn_curpos_inline - 1 ] == '\r' ) ) {
                conn_read_buf[ conn_curpos_inline-1 ] = '\0';
                conn_read_buf[ conn_curpos_inline++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//读数据
http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;
    while (((conn_main_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
                || ((line_status = parse_line()) == LINE_OK)) {

        text = get_line();
        conn_start_line = conn_curpos_inline;
        LOG_INFO("%s", text);
        switch ( conn_main_state ) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line( text );
                if ( ret == BAD_REQUEST ) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers( text );
                if ( ret == BAD_REQUEST ) {
                    return BAD_REQUEST;
                } else if ( ret == GET_REQUEST ) {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content( text );
                if ( ret == GET_REQUEST ) {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}


void http_conn::process() {
    HTTP_CODE read_result = process_read();
    if ( read_result == NO_REQUEST ) {
        modfd( conn_epollfd, conn_sockfd, EPOLLIN );
        return;
    }
    
    bool write_result = process_write( read_result );
    if ( !write_result ) {
        close_conn();
    }
    modfd( conn_epollfd, conn_sockfd, EPOLLOUT);
}