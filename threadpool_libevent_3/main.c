#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "thpool.h"
#include <pthread.h>

#define MAX_LINE    256

typedef struct _TcpMsgHead_s
{
    char flag[6];
    unsigned char progressCode;
    unsigned char actionCode;
    unsigned int  len;
} TcpMsgHead;

typedef struct _TcpMsg_s
{
    TcpMsgHead head;
    void *msg;
    unsigned short crc;
} TcpMsg;


typedef struct _TaskArg_s
{
    struct bufferevent *bev;
    unsigned char *pdata;
} TaskArg;


int verify_msg_crc(TcpMsg *msg)
{
    unsigned short crc_calc = 0;
    unsigned short crc_get = 0;
    char*  ptr = (char *)msg;//璁＄畻CRC浠庡抚澶村紑濮嬭绠楃殑
    memcpy(&crc_get,((char *)msg + sizeof(TcpMsgHead)+msg->head.len),2);//鏈€鍚庝袱涓瓧鑺備负CRC
    crc_calc = CreateCRCCode(ptr,msg->head.len+sizeof(TcpMsgHead));//璁＄畻CRC鐨勯暱搴﹀寘鎷抚澶?
    if(crc_calc != crc_get)
    {
        return 1;
    }
    return 0;
}

int count = 0;

threadpool thpool;

pthread_mutex_t g_ptu_mutex = PTHREAD_MUTEX_INITIALIZER;

void task(void *arg){

    TaskArg *targs = (TaskArg *)arg;
    char buf[] = {"hello,hahah\n"};
  
    printf("Thread #%u working....\n", (int)pthread_self());
    pthread_mutex_lock(&g_ptu_mutex);
    count++;
    printf("recv %d packet\n",count); 
    pthread_mutex_unlock(&g_ptu_mutex);
    //向客户端返回消息
    bufferevent_write(targs->bev,buf,strlen(buf));
    free(targs->pdata);
    free(targs);   
}

void read_cb(struct bufferevent *bev, void *arg) {
    struct evbuffer *buf = bufferevent_get_input(bev);
    evutil_socket_t fd = bufferevent_getfd(bev);

    TcpMsg tcpMsg ;
    int packet_len = 0,ret,len;
    int flag = 0;

    //接下来分析帧
    while(1)
    {  
	len = evbuffer_get_length(buf);
	if(len < sizeof(tcpMsg))
	{
		break;//length small than packet head
	}
        ret = evbuffer_copyout(buf,&tcpMsg,sizeof(tcpMsg));//只读帧头，buf内的计数不改变
        if(ret == sizeof(tcpMsg))
        {
            packet_len = tcpMsg.head.len + sizeof(TcpMsgHead) + 2;
            if(evbuffer_get_length(buf) >= packet_len)
            {
                if(0 != memcmp(tcpMsg.head.flag,"PTUIDN",sizeof(tcpMsg.head.flag)))
                {
                    printf("wrong msg head,disconnect\n");
                    flag = 1;
                    break;
                }
                unsigned char *pdata = (unsigned char *)malloc(packet_len);
                if(NULL == pdata)
                {
                    printf("malloc failed,disconnect\n");
                    flag = 1;
                    break;
                }		
                //copy data
                evbuffer_remove(buf,pdata,packet_len);//从buf内部读数据，buf内的计数改变
                if(0 != verify_msg_crc((TcpMsg *)pdata))
                {
                    printf("wrong msg crc,disconnect\n");
                    flag = 1;
                    break;
                }
		TaskArg *args = (TaskArg *)malloc(sizeof(TaskArg));
                if(NULL == args)
                {
                    printf("malloc failed,disconnect\n");
                    flag = 1;
                    free(pdata);
                    break;
                }
		        args->bev = bev;
                args->pdata = pdata;
                thpool_add_work(thpool, task, (void*)args);               
            }
            else
            {
                //not a full packet
                break;
            }
        }
        else
        {
            break;//data not enough
        }
    }
    if(flag)
    {
        printf("client data error\n");
        close(fd);//关闭socket
        //清空缓冲区
        bufferevent_free(bev);
    }   
}
void write_cb(struct bufferevent *bev, void *arg) {}
void error_cb(struct bufferevent *bev, short event, void *arg) {
    evutil_socket_t fd = bufferevent_getfd(bev);
    printf("fd = %u, ", fd);
    if (event & BEV_EVENT_TIMEOUT) {
        printf("Timed out\n");  //接收超时的时候会进入到这个分支，从而关闭这个连接
    } else if (event & BEV_EVENT_EOF) {
        printf("connection closed\n");
    } else if (event & BEV_EVENT_ERROR) {
        printf("some other error\n");
    }
    close(fd);//关闭socket
    bufferevent_free(bev);
}

//回调函数，用于监听连接进来的客户端socket
void do_accept(evutil_socket_t fd, short event, void *arg) {
    int client_socketfd;//客户端套接字
    struct sockaddr_in client_addr; //客户端网络地址结构体
    int in_size = sizeof(struct sockaddr_in);
    //客户端socket
    client_socketfd = accept(fd, (struct sockaddr *) &client_addr, &in_size); //等待接受请求，这边是阻塞式的
    if (client_socketfd < 0) {
        puts("accpet error");
        exit(1);
    }

    //类型转换
    struct event_base *base_ev = (struct event_base *) arg;

    //socket发送欢迎信息
    char * msg = "Welcome to My socket";
    int size = send(client_socketfd, msg, strlen(msg), 0);


    //创建一个bufferevent
    struct bufferevent *bev = bufferevent_socket_new(base_ev, client_socketfd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    //设置读取方法和error时候的方法，将buf缓冲区当参数传递
    bufferevent_setcb(bev, read_cb, NULL, error_cb, NULL);
    //设置类型
    bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
    //设置水位
    bufferevent_setwatermark(bev, EV_READ, 0, 0);
    //const struct timeval read_tv = { 5, 0 };
    //bufferevent_set_timeouts(bev, &read_tv, NULL);//设置接收超时时间为5秒，如果是主从同步则不需要加接收超时
}

struct event_base *base_ev;
struct event *ev;

void exit_handler(int s)
{
    printf("in handler \n");

    //销毁event_base
    event_base_free(base_ev);
    event_free(ev);

    thpool_wait(thpool);
    puts("Killing threadpool");
    thpool_destroy(thpool);
}

//入口主函数
int main() {

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGKILL, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);

    int server_socketfd; //服务端socket
    struct sockaddr_in server_addr;   //服务器网络地址结构体
    memset(&server_addr,0,sizeof(server_addr)); //数据初始化--清零
    server_addr.sin_family = AF_INET; //设置为IP通信
    server_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
    server_addr.sin_port = htons(8001); //服务器端口号

    //创建服务端套接字
    server_socketfd = socket(PF_INET,SOCK_STREAM,0);
    if (server_socketfd < 0) {
        puts("socket error");
        return 0;
    }

    evutil_make_listen_socket_reuseable(server_socketfd); //设置端口重用
    evutil_make_socket_nonblocking(server_socketfd); //设置无阻赛

    //绑定IP
    if (bind(server_socketfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))<0) {
        puts("bind error");
        return 0;
    }

    puts("Making threadpool with 4 threads");
    thpool = thpool_init(4);  

    //监听,监听队列长度 5
    listen(server_socketfd, 10);

    //创建 event_base 事件的集合，多线程的话 每个线程都要初始化一个event_base
    evthread_use_pthreads();
    base_ev = event_base_new();
    const char *x =  event_base_get_method(base_ev); //获取IO多路复用的模型，linux一般为epoll
    printf("METHOD:%s\n", x);

    //创建一个事件，类型为持久性EV_PERSIST，回调函数为do_accept（主要用于监听连接进来的客户端）
    //将 base_ev 传递到 do_accept 中的arg参数
    
    ev = event_new(base_ev, server_socketfd, EV_TIMEOUT|EV_READ|EV_PERSIST, do_accept, base_ev);

    //注册事件，使事件处于 pending的等待状态
    event_add(ev, NULL);

    //事件循环
    event_base_dispatch(base_ev);

    return 1;
}

