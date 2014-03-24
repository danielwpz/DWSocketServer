#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include "DWSocketServer.h"

using namespace std;

std::mutex print_mutex;

void handler(DWSocketServer *svr, int fd)
{
	// std::unique_lock<std::mutex> ulock(print_mutex);

	int i, r;
	std::string str;
	cout << "Incoming from (" << fd << ") ";

	for (i = 0; i < 2; i++) {
		str = "";
		// while((r = svr->sread(fd, str)) == 0){}
		cout << "[" << std::this_thread::get_id() << "]<\n";
		r = svr->sread(fd, str);
		if (r < 0) {
			return;
		}

		cout << str << "@" << endl;

		std::string reply("Server reply :)");
		svr->swrite(fd, reply);
		cout << ">[" << std::this_thread::get_id() << "]\n";
	}
}

int main()
{
	DWSocketServer server(6060);

	server.setHandler(handler);

	server.startup();

	return -1;
}
