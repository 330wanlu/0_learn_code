#include <event2/event.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
 
//保存所有fd对应的event结构体指针.方便销毁事件.
//一些事件的销毁这里就不做处理了.实际程序中注意内存泄露.当事件已经无用后记得销毁掉.
struct event * fdevent_array[10000];
 
void setnonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
 
void reuseAddr(int fd) {
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
}
 
void read_cb(evutil_socket_t fd, short events, void *arg)
{
	struct event * myevent;
	
	if ( events & EV_READ ){
		//真正的应用中这里需要两个用户的buffer来保存该连接上的数据.
		char buffer[1024];
		while(1){
			memset( buffer , 0 , 1024);
			int nbytes = read( fd , buffer , 1023);
			if ( nbytes < 0){
				if (errno == EINTR) continue;
				if (errno == EAGAIN) break;
				printf("client error , fd [%d]\n" , fd);
				goto fdclose;
				break;
			}
			if ( nbytes == 0){
				printf("client close , fd [%d]\n" , fd);
				goto fdclose;
				break;
			}
			printf("data from client: %s" , buffer);
			write( fd , buffer , nbytes);
		}
	}
	return;
	
fdclose:
	//
	myevent = fdevent_array[fd];
	event_del(myevent);
	event_free(myevent);
	close(fd);
	fdevent_array[fd] = NULL;
}
 
void listen_cb(evutil_socket_t listener, short events , void *arg)
{
	if (events & EV_TIMEOUT ){
		printf("timer event\n");
	}
	if (events & EV_READ ){
		struct event_base *base = arg;
		struct sockaddr_storage ss;
		socklen_t slen = sizeof(ss);
		int fd = accept(listener, (struct sockaddr*)&ss, &slen);
		//接受了一个新客户连接,我们为这个新连接设置一个事件
		if(fd > 0){
			printf("accept new client fd [%d]\n" , fd);
			setnonblock( fd );
			//我们现在只关注读事件,将客户端发来的信息回送给客户端.
			struct event * client_event = event_new( base , fd , EV_READ|EV_PERSIST , read_cb , (void*)base  );
			event_add( client_event , NULL);
			fdevent_array[fd] = client_event;
		}
	}
}
 
void sigint_cb(evutil_socket_t sig, short events , void *arg){
	struct event_base *base = arg;
	if ( events & EV_SIGNAL ){
		printf("capture signal [%d]\n" , sig);
		event_base_loopbreak(base);
	}
}
 
int main(){
	//
	struct event_base * base = event_base_new();
	
	//以下代码开启server监听一个本地端口.可以将ip改为自己的服务器以方便测试.
	int port = 9999;
	struct sockaddr_in my_addr;
	memset( &my_addr , 0 , sizeof(struct sockaddr_in));
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(port);
	my_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	int listen_socket = socket(AF_INET , SOCK_STREAM , 0);
	setnonblock(listen_socket);
	reuseAddr(listen_socket);
	bind(listen_socket , (struct sockaddr*)&my_addr , sizeof(struct sockaddr));
	listen(listen_socket , 10);
	
	//给监听的socket添加一个事件,用来accept新连接,关注读事件,并将该event设置为永久事件.设置好回调函数和参数.
	struct event *listener_event = event_new( base , listen_socket , EV_READ|EV_PERSIST , listen_cb , (void*)base);
	//将监听事件加入事件循环.同时我们在监听事件上加一个定时器事件.在回调函数中判断应该接受连接还是定时器触发.
	struct timeval expire = { 5 , 0 };
	event_add( listener_event , &expire);
	fdevent_array[listen_socket] = listener_event;
	
	//设置一个信号事件,我们监听(ctrl+c的中断信号),收到信号后打印一条消息,退出事件循环.
	struct event *signal_event = evsignal_new( base , SIGINT , sigint_cb , (void*)base);
	evsignal_add( signal_event , NULL );
	
	//开始事件循环
	event_base_dispatch( base );
	
	//
	event_base_free(base);
}