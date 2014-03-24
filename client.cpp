#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <string.h>
#include <strings.h>

#define IP "127.0.0.1"
#define PORT 6060

using namespace std;

inline void errlog()
{
	int __errno = errno;
	char *str = strerror(__errno);

	cout << str << endl;
}

void thread_sleep(int interval)
{
	std::chrono::milliseconds dura(interval);
	std::this_thread::sleep_for(dura);
}

void client_fun(int n)
{
	int fd;
	int i, r;
	struct sockaddr_in clientAddr;
	char *buf = new char[300];
	char *rbuf = new char[128];

	for (i = 0; i < 1; i++) {
		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			errlog();
			cout << "ERROR: socket() failed.\n";
			return;
		}

		bzero(&clientAddr, sizeof(clientAddr));
		clientAddr.sin_family = AF_INET;
		clientAddr.sin_addr.s_addr = inet_addr(IP);
		clientAddr.sin_port = htons(PORT);

		if (connect(fd, (struct sockaddr *)&clientAddr,
					sizeof(clientAddr)) == -1) {
			errlog();
			cout << "ERROR: connect() failed.\n";
			return;
		}

		sprintf(buf, "[%d]Client to Server", n);
		std::string str(254, '0');
		str += '1';
		const char *abuf = str.c_str();
		if (write(fd, abuf, strlen(abuf)) < 0) {
			errlog();
			cout << "ERROR: write() failed.\n";
			close(fd);
			return;
		}
		// cout << "write 1\n";

		r = read(fd, rbuf, 19);
		if (r > 0) {
			rbuf[r] = '\0';
			cout << rbuf << endl;
		}else {
			errlog();
			cout << "ERROR: read() failed.\n";
		}
		
		thread_sleep(5000);	// 100s

		sprintf(buf, "[%d]Client to Server again", n);
		if (write(fd, buf, strlen(buf)) < 0) {
			errlog();
			cout << "ERROR: write() failed.\n";
			close(fd);
			return;
		}
		// cout << "write 1\n";
		

		r = read(fd, rbuf, 19);
		if (r > 0) {
			rbuf[r] = '\0';
			cout << rbuf << endl;
		}else {
			errlog();
			cout << "ERROR: read() failed.\n";
		}

		// thread_sleep(300 * 1000);	// 300s

		close(fd);
	}

	delete buf;
	delete rbuf;
}

int main(int argc, char **argv)
{
	int i, j;
	int n = 10;
	std::vector<std::thread> vec;

	if (argc == 2) {
		n = atoi(argv[1]);
	}

	try{
		for (i = 0; i < n; i++) {
			vec.push_back(std::thread(std::bind(&client_fun, i)));
			if (i % 500 == 0)
				thread_sleep(100); // 0.1s
		}
	}catch(std::exception e) {
		cout << e.what() << endl;
	}

	for (j = 0; j < i; j++) {
		vec[j].join();
	}

	return 0;
}
