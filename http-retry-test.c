#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>

#include <assert.h>
#include <unistd.h>
#include <pthread.h>

struct Args
{
	/** http */
	ev_uint16_t port;

	struct {
		struct event_base *base;
	} client;
	struct {
		struct event_base *base;
	} server;
};

static void
http_dispatcher_test_done(struct evhttp_request *req, void *arg)
{
	struct Args *args = (struct Args *)arg;

	puts("Page downloaded, stopping event bases");
	event_base_loopexit(args->client.base, NULL);
	event_base_loopexit(args->server.base, NULL);
}
void *client(void *arg)
{
	struct Args *args = (struct Args *)arg;
	struct event_base *base;
	struct evhttp_connection *evcon;
	struct evhttp_request *req;
	const struct timeval tv_timeout = { 0, 500000 };
	const struct timeval tv_retry = { 0, 500000 };

	base = event_base_new();
	assert(base);
	args->client.base = base;

	evcon = evhttp_connection_base_new(base, NULL, "127.0.0.1", args->port);
	assert(evcon);

	evhttp_connection_set_retries(evcon, 3);
	evhttp_connection_set_timeout_tv(evcon, &tv_timeout);
	evhttp_connection_set_initial_retry_tv(evcon, &tv_retry);

	req = evhttp_request_new(http_dispatcher_test_done, args);
	assert(req);

	assert(evhttp_make_request(evcon, req, EVHTTP_REQ_GET, "/") != -1);

	event_base_dispatch(base);
	puts("Client stopped");

	evhttp_connection_free(evcon);
	event_base_free(base);

	return NULL;
}

void http_dispatcher_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = evbuffer_new();
	evbuffer_add_printf(evb, "DISPATCHER_TEST");
	evhttp_send_reply(req, HTTP_OK, "Everything is fine", evb);
	evbuffer_free(evb);
}
void *server(void *arg)
{
	struct Args *args = (struct Args *)arg;
	struct evhttp *http;
	struct evhttp_bound_socket *sock;
	struct event_base *base;

	base = event_base_new();
	assert(base);
	args->server.base = base;

	http = evhttp_new(base);
	assert(http);

	sock = evhttp_bind_socket_with_handle(http, "127.0.0.1", args->port);
	assert(sock);

	evhttp_set_cb(http, "/", http_dispatcher_cb, args);
	event_base_dispatch(base);
	puts("Server stopped");

	evhttp_free(http);
	event_base_free(base);

	return NULL;
}

int main(int argc, char **argv)
{
	struct Args args = { .port = 2048 };
	pthread_t clientThread, serverThread;

	assert(!pthread_create(&clientThread, NULL, client, &args));
	assert(!pthread_create(&serverThread, NULL, server, &args));

	assert(!pthread_join(clientThread, NULL));
	assert(!pthread_join(serverThread, NULL));

	return 0;
}
