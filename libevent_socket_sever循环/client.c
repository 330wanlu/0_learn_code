#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<event.h>
#include<event2/util.h>


int connect_server(const char* server_ip, int port);


void cmd_read_data(int fd, short events, void* arg);
void socket_read_data(int fd, short events, void *arg);

int main(int argc, char** argv)
{
    if( argc < 3 )
    {
        printf("please input 2 parameters\n");
        return -1;
    }


    //两个参数依次是服务器端的IP地址、端口号
    int sockfd = connect_server(argv[1], atoi(argv[2]));
    if( sockfd == -1)
    {
        perror("tcp_connect error ");
        return -1;
    }

    printf("connect to server successfully\n");

    struct event_base* base = event_base_new();

    struct event *ev_sockfd = event_new(base, sockfd,                      //监视客户端套接字，一旦可读就读出
                                        EV_READ | EV_PERSIST,
                                        socket_read_data, NULL);
    event_add(ev_sockfd, NULL);

    //监听终端输入事件
    struct event* ev_cmd = event_new(base, STDIN_FILENO,                     //监视标准输入，等待输入字符串
                                      EV_READ | EV_PERSIST, cmd_read_data,
                                      (void*)&sockfd);


    event_add(ev_cmd, NULL);

    event_base_dispatch(base);

    printf("finished \n");
    return 0;
}


void cmd_read_data(int fd, short events, void* arg)
{
    char msg[1024];

    int ret = read(fd, msg, sizeof(msg)-1);
    if( ret <= 0 )
    {
        perror("read fail ");
        exit(1);
    }

    int sockfd = *((int*)arg);
    if(msg[ret - 1]=='\n') msg[ret - 1] = '\0';
    else msg[ret] = '\0';
    //把终端的消息发送给服务器端,客户端忽略性能考虑，直接利用阻塞方式发送
    printf("write to server>>> [%s]\n", msg);
    write(sockfd, msg, ret);
}


void socket_read_data(int fd, short events, void *arg)
{
    char msg[1024];

    //为了简单起见，不考虑读一半数据的情况
    int len = read(fd, msg, sizeof(msg)-1);
    if( len == 0 )
    {
        printf("connection close. exit~\n");
        exit(1);
    }else if(len < 0){
                perror("read fail ");
                return ;
        }

    msg[len] = '\0';

    printf("recv from server<<<<< [%s] \n", msg);
}



typedef struct sockaddr SA;
int connect_server(const char* server_ip, int port)
{
    int sockfd, status, save_errno;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr) );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    status = inet_aton(server_ip, &server_addr.sin_addr);

    if( status == 0 ) //the server_ip is not valid value
    {
        errno = EINVAL;
        return -1;
    }

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if( sockfd == -1 )
        return sockfd;


    status = connect(sockfd, (SA*)&server_addr, sizeof(server_addr) );

    if( status == -1 )
    {
        save_errno = errno;
        close(sockfd);
        errno = save_errno; //the close may be error
        return -1;
    }

    //evutil_make_socket_nonblocking(sockfd);

    return sockfd;
}
