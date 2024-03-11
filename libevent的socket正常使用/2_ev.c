#include <string.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include<sys/signal.h>
#include<event.h>

void on_accept(evutil_socket_t listener, short event, void *arg)
{
    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);

    if (fd < 0) {
        fprintf(stderr, "Failed to accept client connection: %s\n",
                strerror(errno));
        return;
    }

    char ip[INET6_ADDRSTRLEN];
    void *addr;
    if (ss.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&ss;
        addr = &(s->sin_addr);
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&ss;
        addr = &(s->sin6_addr);
    }
    inet_ntop(ss.ss_family, addr, ip, sizeof(ip));

    printf("Accepted connection from %s\n", ip);

    // 将新连接加入事件循环，等待读取数据
    struct event *ev = event_new(base, fd, EV_READ | EV_PERSIST, on_read, base);
    event_add(ev, NULL);
}

void on_read(evutil_socket_t fd, short event, void *arg)
{
    struct event_base *base = arg;
    char buf[1024];
    size_t n = recv(fd, buf, sizeof(buf), 0); //read 读

    if (n == 0) {
        printf("Connection closed.\n");
    } else if (n < 0) {
        fprintf(stderr, "Error on read: %s\n", strerror(errno));
    } else {
        printf("Received %zu bytes: %.*s\n", n, (int)n, buf);

        // 回写给客户端
        send(fd, buf, n, 0); //write 写
    }

    // 从事件循环中删除这个事件
    event_del(event_get_struct_eventarg((struct event*)arg));
    event_free((struct event*)arg);
    close(fd);
}

int main()
{   
    //创建用于监听的文件描述符sockfd，并进行bind，listen
    //这里我是伪代码
    int sockfd=Sock_Init();
    //创建libevent实例
    struct event_base*base=event_init();
    //为用于监听的文件描述创建事件对象，如果该文件描述符对应事件发生则会执行回调函数

    //需要把base传入，发生事件（有客户端连接）在回调函数中需要接受该描述符并添加到base（libevent实例中）
    struct event*p=event_new(base,sockfd,EV_READ||EV_PERSIST,on_accept,base);

    event_add(p,0);
//启动事件循环
    event_base_dispatch(base);
}
