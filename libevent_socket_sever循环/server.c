#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <event.h>
#include <event2/event.h>
#include <assert.h>

#define BUFLEN 128

struct ConnectData{                         //用来保存事件的结构体

      char buff[BUFLEN];                   //用来保存字符串的字符数组
      struct event* ev;                    //事件指针
};


void accept_connection(int fd,short events,void* arg);                  //接收到客户端请求后，把客户端套接字加入到事件集中并进行监听
void do_echo_request(int fd,short events,void* arg);                   //读出客户端写入的数据，并监听写事件
void do_echo_response(int fd,short events,void* arg);                   //向客户端写入数据并监听读事件
int tcp_server_init(int port,int listen_num);                          //接受客户端连接函数，进行服务器端的套接字创建

struct event_base* base;                                         //事件集

int main(int argc,char** argv){

    int listener=tcp_server_init(1234,666);
    if(listener==-1){

       printf("服务端套接字创建失败！\n");
       exit(1);
    }

    base=event_base_new();                     //创建事件集

    struct event* event_listen=event_new(base,listener,EV_READ | EV_PERSIST,accept_connection,base);//设置事件，第一个参数为事件集，第二个为监听的套接字，第三个为事件行为，EV_READ | EV_PERSIST 表示一旦可读就执行绑定函数，第四个参数为事件发生时执行的函数，第五个为传入执行函数的参数。

    event_add(event_listen,NULL);                  //把事件加入到事件集中

    event_base_dispatch(base);                     //循化处理事件

    return 0;
}


int tcp_server_init(int port,int listen_num){

    evutil_socket_t listener;           //为跨平台定义的类型，在这里可以等同为int

    listener=socket(AF_INET,SOCK_STREAM,0);
    if(listener==-1){
       return -1;
    }

    evutil_make_listen_socket_reuseable(listener);                    //设置套接字可重复绑定

    struct sockaddr_in server;
    server.sin_family=AF_INET;
    // server.sin_addr.s_addr=0;
    server.sin_addr.s_addr=inet_addr("127.0.0.1");  //具体的IP地址
    server.sin_port=htons(port);

    if(bind(listener,(struct sockaddr*)&server,sizeof(server))<0){

       printf("服务器端绑定失败！\n");
       exit(2);
    }

    if(listen(listener,listen_num)<0){

       printf("服务器端监听失败！\n");
       exit(3);
    }

    evutil_make_socket_nonblocking(listener);                      //设置非阻塞

    return listener;
}

void accept_connection(int fd,short events,void* arg){

     evutil_socket_t sockfd;

     struct sockaddr_in client;
     socklen_t len=sizeof(client);

     sockfd=accept(fd,(struct sockaddr*)&client,&len);
     evutil_make_socket_nonblocking(sockfd);
     printf("收到一个新的连接！\n");

     struct event_base* base=(struct event_base*)arg;

     struct ConnectData* tmp=(struct ConnectData*)malloc(sizeof(struct ConnectData));              //ConnectData结构体，存放事件与字符串

     struct event *ev = event_new(NULL, -1, 0, NULL, NULL);                                 //创建一个事件，为了存放到上述结构体中
     tmp->ev=ev;

     event_assign(ev, base, sockfd, EV_READ,do_echo_request, (void*)tmp);                   //设置事件参数
     event_add(ev,NULL);                                                                   //加入到事件集中

     return;
}

void do_echo_request(int fd,short events,void* arg){


     struct ConnectData* tmp=(struct ConnectData*)arg;

     int ret=read(fd,tmp->buff,BUFLEN-1);                        //客户端套接字可读，读出数据
     if(ret<0){

        printf("读取失败！\n");
        exit(4);
     }

     char *str=tmp->buff;
     printf("-------%s-------\n",str);

     struct event* ev=tmp->ev;
     event_set(ev,fd,EV_WRITE,do_echo_response,(void*)tmp);           //修改事件，监听事件可写

     ev->ev_base=base;

     event_add(ev,NULL);

     return;
}

void do_echo_response(int fd,short events,void* arg){


     struct ConnectData* tmp=(struct ConnectData*)arg;

     int ret=write(fd,tmp->buff,sizeof(tmp->buff));                        //将上个函数中读出的数据发回到客户端
     if(ret<0){

        printf("数据写入失败！\n");
        exit(5);
     }

     struct event* ev=tmp->ev;

     event_set(ev,fd,EV_READ,do_echo_request,(void*)tmp);                   //修改事件监听写端

     ev->ev_base=base;

     event_add(ev,NULL);

     bzero(tmp->buff,sizeof(tmp->buff)); //建议用memset                         //清空字符数组

     return;
}

