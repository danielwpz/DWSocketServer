#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include "DWSocketServer.h"

using namespace std;

void handler(int fd, string content)
{
	cout << "Incoming from [" << fd << "] ";
	cout << content << endl;
	
	char buf[] = "server reply :)";

	if (write(fd, buf, 15) < 0) {
		cout << "ERROR: write() failed.\n";
	}

	std::chrono::milliseconds dura(1 * 100);
	std::this_thread::sleep_for(dura);	
}

int main()
{
	DWSocketServer server(6060);

	server.setHandler(handler);

	server.startup();

	return -1;
}
