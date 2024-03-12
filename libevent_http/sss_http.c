#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>

void http_request_handler(struct evhttp_request *req, void *arg) {
    struct evbuffer *buf = evbuffer_new();
    if (buf == NULL) {
        perror("Failed to create response buffer");
        return;
    }

    // 设置 HTTP 响应头
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain");

    // 发送 HTTP 响应
    evbuffer_add_printf(buf, "Hello from HTTP server\n");
    evhttp_send_reply(req, 200, "OK", buf);

    evbuffer_free(buf);
}

int main() {
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Error creating event base\n");
        return -1;
    }

    struct evhttp *httpd = evhttp_new(base);
    if (!httpd) {
        fprintf(stderr, "Error creating evhttp structure\n");
        return -1;
    }

    evhttp_set_gencb(httpd, http_request_handler, NULL);

    if (evhttp_bind_socket(httpd, "0.0.0.0", 8080) != 0) {
        fprintf(stderr, "Error binding server socket\n");
        return -1;
    }

    event_base_dispatch(base);

    evhttp_free(httpd);
    event_base_free(base);

    return 0;
}
