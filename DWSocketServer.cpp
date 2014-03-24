/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 danielwpz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <iostream>
#include "DWSocketServer.h"

#ifdef NONBLOCK_SOCKET
#define BUF_SIZE 256
#else
#define BUF_SIZE 768
#endif

#define EPOLL_MAX_EVENTS 128
#define EPOLL_QUEUE_SIZE ((8 >> TIME_COST_RATE)? \
								(8 >> TIME_COST_RATE) : 1)

#define THREAD_POOL_SIZE (DEFAULT_BURDEN_POOL_SIZE * CONCURRENCY_RATE)

using namespace std;


inline void log(std::string str, bool verbose = true)
{
	if (verbose) {
		cout << "server: " << str << flush;
	}else {
		cout << str << flush;
	}
}

inline void errlog(std::string str)
{
	int __errno = errno;
	char *__err_des = strerror(__errno);

	log("ERROR: ");
	log(str + " ##errno: " + __err_des + " ##\n", false);
}

DWSocketServer::DWSocketServer(short port)
{
	int r;
	struct sockaddr_in serverAddr;

	/**
	 * init DWThreadPool
	 */
	cout << THREAD_POOL_SIZE << endl;
	pool = new DWThreadPool(THREAD_POOL_SIZE,
							THREAD_POOL_SIZE);

	/**
	 * init listen socket
	 */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	// set fast reboot
	r = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &r, sizeof(r));

	// set port and address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);	
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(serverAddr.sin_zero), 0, 8);
	// bind
	r = bind(listenfd, (struct sockaddr*) &serverAddr,
				sizeof(struct sockaddr));
	if (r == -1) {
		errlog("bind() failed.");
		delete pool;

		return;
	}

	/**
	 * init epoll
	 */
	epollfd = epoll_create(EPOLL_MAX_EVENTS);
	if (epollfd <= 0) {
		errlog("epoll_create() failed.");
		delete pool;
		close(listenfd);

		return;
	}
}

DWSocketServer::~DWSocketServer()
{

}

int DWSocketServer::setHandler(SocketHandler newHandler)
{
	handler = newHandler;

	return 0;
}

int DWSocketServer::startup()
{
	int i, r;
	int clientfd;
	struct sockaddr_in clientAddr;
	struct epoll_event ev;

	if (!handler) {
		errlog("register handler first.\n");
		return E_BAD_HDL;
	}

	// listen
	r = listen(listenfd, SOMAXCONN);
	if (r == -1) {
		errlog("Failed to listen.\n");
		return E_LSN;
	}

	// start the thread pool
	pool->start();
	for (i = 0; i < THREAD_POOL_SIZE; i++) {
		pool->run(std::bind(&DWSocketServer::multiEpoll,
							this));
	}

	// accept
	int clientAddrSize = sizeof(struct sockaddr_in);
	while (1) {
		clientfd = accept(listenfd, (struct sockaddr*) &clientAddr,
							(socklen_t *) &clientAddrSize);	
		if (clientfd == -1) {
			char *errmsg = strerror(errno);
			cout << errmsg;
			errlog("accept() error.\n");
			continue;
		}

#ifdef NONBLOCK_SOCKET
		// set clientfd as non-block
		int flags = fcntl(clientfd, F_GETFL, 0);
		fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);
#endif

		// add clientfd to epoll queue
		ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
		ev.data.fd = clientfd;

		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) < 0) {
			errlog("epoll_ctl() failed.\n");
			continue;
		}
	}

	return 0;
}	

int DWSocketServer::shutdown()
{

	return 0;
}

/**
 * Task assigned to each working thread.
 */
void DWSocketServer::multiEpoll()
{
	int i, r;
	struct epoll_event events[EPOLL_QUEUE_SIZE];
	struct epoll_event ev;
	char buf[BUF_SIZE];
	
	while(1) {
		int nfds = epoll_wait(epollfd, events, EPOLL_QUEUE_SIZE, -1);
		if (nfds <= 0) {
			errlog("epoll_wait() failed. ");
			continue;
		}

		cout << "[" << std::this_thread::get_id() << "]";
		cout << "wait OK.\n";
		for (i = 0; i < nfds; i++) {
			if (events[i].events & EPOLLRDHUP) {
				log("Opposite side close. EPOLLRDHUP\n");
				sclose(events[i].data.fd);
				continue;
			}else if (events[i].events & EPOLLIN) {
				// call the user-defined handler
				handler(this, events[i].data.fd);

				// rearm event source in epoll queue
				ev.events = EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
				ev.data.fd = events[i].data.fd;

				if (epoll_ctl(epollfd, EPOLL_CTL_MOD, 
								ev.data.fd, &ev) < 0) {
					errlog("epoll_ctl() modify failed.\n");
				}
			}else {
				// TODO break or err
			}
		}
	}

}

/**
 * non-blocking mode:
 *	 sread() works fine with non-block socket.
 *	 But if you call it not after epoll_wait(),
 *	 you have to make sure that there is some-
 *	 thing really in the buffer and can be read.
 *	 If you believe the message could be delivered
 *	 in very short period of time, just spin it.
 *
 * blocking mode:
 *   sread() works almost fine with blocking mode.
 *   By 'almost', I meant that in this case sread()
 *   will return as soon as it got (BUF_SIZE - 1)
 *   bytes, regardless of any pending message.
 *   So, set the BUF_SIZE a litter bit larger.
 *   
 * Returns:
 *   > 0 - number of bytes actually read
 *   = 0 - nothing has been read (non-blocking only)
 *   < 0 - error happened
 */ 
int DWSocketServer::sread(int fd, std::string &str)
{
	int r, cnt;
	char buf[BUF_SIZE];
	cnt = 0;

	while(1) {
		r = read(fd, buf, BUF_SIZE - 1);

		if (r > 0 && r < BUF_SIZE - 1) {
			// terminate
			cnt += r;
			buf[r] = '\0';
			str.append(buf);
			break;
		}else if (r == BUF_SIZE - 1) {
			cnt += r;
			buf[r] = '\0';
			str.append(buf);
#ifndef NONBLOCK_SOCKET
			break;
#endif
		}else if (r < 0 && errno == EAGAIN) {
			break;
		}else if (r < 0) {
			cnt = -1;
			break;
		}else if (r == 0) {
			cnt = -1;
			break;
		}
	}

	return cnt;
}

int DWSocketServer::swrite(int fd, const std::string &str)
{
	int r;
	const char *buf = str.c_str();

	r = write(fd, buf, str.length());
	if (r < 0) {
		errlog("ERROR: write failed.\n");
	}

	return r;
}

void DWSocketServer::sclose(int fd)
{
	struct epoll_event ev;

	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev) < 0) {
		errlog("epoll_ctl() DEL failed.\n");
	}

	if (close(fd) < 0) {
		errlog("close() failed.");
	}
}
