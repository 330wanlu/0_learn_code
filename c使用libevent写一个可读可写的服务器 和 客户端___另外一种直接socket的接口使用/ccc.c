#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

void read_cb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    char *data = malloc(len + 1);
    
    evbuffer_remove(input, data, len);
    data[len] = '\0';
    printf("Received: %s\n", data);
    free(data);
}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_CONNECTED) {
        printf("Connected to server.\n");
        // 向服务器发送数据
        const char *msg = "Hello, Server!";
        bufferevent_write(bev, msg, strlen(msg));
    } else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        printf("Connection closed or encountered an error.\n");
        event_base_loopbreak(bufferevent_get_base(bev));
    }
}

int main() {
    struct event_base *base = event_base_new();

    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = htons(8888);

    bufferevent_socket_connect(bev, (struct sockaddr*)&sin, sizeof(sin));

    event_base_dispatch(base);

    bufferevent_free(bev);
    event_base_free(base);

    return 0;
}




/*

#include <stdio.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

void read_cb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    char *data = malloc(len + 1);

    evbuffer_remove(input, data, len);
    data[len] = '\0';

    printf("Received data from server: %s\n", data);

    free(data);
}

void write_cb(struct bufferevent *bev, void *ctx) {
    // Write callback implementation (optional)
}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR) {
        fprintf(stderr, "Error on the connection\n");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

void connect_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_CONNECTED) {
        char data[] = "Hello, Server!";
        bufferevent_write(bev, data, sizeof(data));
    }
}

int main() {
    struct event_base *base = event_base_new();
    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);

    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin));
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);

    event_base_dispatch(base);

    event_base_free(base);

    return 0;
}


*/
