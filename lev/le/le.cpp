// le.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <signal.h>

#ifndef WIN32
#define WIN32
#endif

#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/listener.h"
#include "event2/util.h"
#include "event2/event.h"

#pragma comment(lib,"libevent.lib")
#pragma comment(lib,"libevent_core.lib")
#pragma comment(lib,"libevent_extras.lib")

#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#endif

static const int PORT = 9995;

static void conn_readcb(struct bufferevent *bev, void *user_data)
{
//	struct evbuffer *input = bufferevent_get_input(bev);
//	if (evbuffer_get_length(input) == 0) {
//		printf("conn_readcb flushed answer\n");
//		bufferevent_free(bev);
//	}
#define MAX_LINE    512
	char line[MAX_LINE + 1];
	int n;
	// -- evutil_socket_t fd = bufferevent_getfd(bev);

	while (n = bufferevent_read(bev, line, MAX_LINE), n > 0) {
		// -- line[n] = '\0';
		// -- printf("fd=%u, read line: %s\n", fd, line);

		bufferevent_write(bev, line, n);
	}
}

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF){
		printf("Connection closed.\n");
	}else if (events & BEV_EVENT_ERROR){
		printf("Got an error on the connection: %s\n",
			strerror(errno));/*XXX win32*/
	}else if (events & BEV_EVENT_READING) {
		printf("BEV_EVENT_READING \n");
	}
	/* None of the other events can happen here, since we haven't enabled
	* timeouts */
	bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = (struct event_base *)user_data;
	struct timeval delay = { 2, 0 };

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay);
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
	struct sockaddr *sa, int socklen, void *user_data)
{
	struct event_base *base = (struct event_base *)user_data;
	struct bufferevent *bev;

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev){
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base);
		return;
	}
	bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
	bufferevent_enable(bev, EV_READ );
}

int main(int argc, char **argv)
{
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2,2), &wsa_data);
#endif

	struct event_base *base = event_base_new();
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;

	struct evconnlistener* listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
		(struct sockaddr*)&sin,
		sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	struct event* signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);

#ifdef WIN32
	WSACleanup();
#endif

	printf("done\n");
	return 0;
}


