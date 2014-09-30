#include <event2/http.h>
#include <event2/event.h>

#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct Args
{
	/** http */
	ev_uint16_t port;

	struct {
		struct event_base *base;
	} client;
};

static void
http_dispatcher_test_done(struct evhttp_request *req, void *arg)
{
	struct Args *args = (struct Args *)arg;

	puts("Page downloaded, stopping event bases");
	event_base_loopexit(args->client.base, NULL);
}
void *client(void *arg)
{
	struct Args *args = (struct Args *)arg;
	struct event_base *base;
	struct evhttp_connection *evcon;
	struct evhttp_request *req;
	const struct timeval tv_timeout = { 0, 500000 };
	const struct timeval tv_retry = { 2, 0 };

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

void *server(void *arg)
{
	struct Args *args = (struct Args *)arg;
	struct sockaddr_in addr;
	int fd, err;
	struct timespec t = { 1 };

	fd = socket(PF_INET, SOCK_STREAM|SOCK_CLOEXEC, IPPROTO_IP);
	assert(fd >= 0);

	addr.sin_port = ntohs(args->port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_family = AF_INET;
	err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	assert(!err);

	err = listen(fd, 1);
	assert(!err);

	nanosleep(&t, NULL);

	socklen_t addrSize = sizeof(addr);
	int clientFd = accept(fd, (struct sockaddr *)&addr, &addrSize);
	assert(clientFd >= 0);
	close(clientFd);

	close(fd);
	puts("Server stopped");

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
