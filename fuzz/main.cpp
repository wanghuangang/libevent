#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>

struct input
{
	const uint8_t *bytes;
	size_t size;

	int read;
	int write;
	int error;
};

static void readcb(struct bufferevent *be, void *arg)
{
	struct input *i = (struct input *)arg;

	struct evbuffer *buf = bufferevent_get_input(be);
	unsigned char *b = evbuffer_pullup(buf, -1);
	if (!strncmp((char *)b, (char *)i->bytes, i->size)) {
		++i->read;
	}
}
static void writecb(struct bufferevent *be, void *arg)
{
	struct input *i = (struct input *)arg;
	++i->write;
}
static void errorcb(struct bufferevent *be, short what, void *arg)
{
	struct input *i = (struct input *)arg;
	++i->error;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *bytes, size_t size)
{
	struct event_base *base;
	struct bufferevent *bes[2];
	struct input input = {
		.bytes = bytes,
		.size = size,

		.read = 0,
		.write = 0,
		.error = 0,
	};

	/** Fine, just ignore it */
	if (!size) {
		return EXIT_SUCCESS;
	}

	evthread_use_pthreads();

	base = event_base_new();
	if (bufferevent_pair_new(base, 0, bes)) {
		return EXIT_FAILURE;
	}

	bufferevent_setcb(bes[0], NULL,   writecb, errorcb, &input);
	bufferevent_setcb(bes[1], readcb, NULL,    errorcb, &input);
	bufferevent_write(bes[0], input.bytes, input.size);
	bufferevent_enable(bes[1], EV_READ);

	if (event_base_dispatch(base) == -1) {
		return EXIT_FAILURE;
	}

	bufferevent_free(bes[0]);
	bufferevent_free(bes[1]);
	event_base_free(base);

	if (!input.read) {
		fputs("No readcb, or invalid data\n", stderr);
		return EXIT_FAILURE;
	}
	if (!input.write) {
		fputs("No writecb\n", stderr);
		return EXIT_FAILURE;
	}
	if (input.error) {
		fputs("No errorcb\n", stderr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
