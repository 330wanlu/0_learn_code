#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>

void http_client_request_done(struct evhttp_request *req, void *arg) {
    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    char data[1024];
    int n;

    while ((n = evbuffer_remove(buf, data, sizeof(data))) > 0) {
        fwrite(data, 1, n, stdout);
    }
}

int main() {
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Error creating event base\n");
        return -1;
    }

    struct evhttp_connection *conn = evhttp_connection_base_new(base, NULL, "127.0.0.1", 8080);
    if (!conn) {
        fprintf(stderr, "Error creating evhttp connection\n");
        return -1;
    }

    struct evhttp_request *req = evhttp_request_new(http_client_request_done, NULL);
    if (!req) {
        fprintf(stderr, "Error creating evhttp request\n");
        return -1;
    }

    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/");
    event_base_dispatch(base);

    evhttp_connection_free(conn);
    event_base_free(base);

    return 0;
}
