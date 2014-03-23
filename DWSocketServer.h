#include <functional>
#include <string>
#include "./ThreadPool/DWThreadPool.h"

#define E_LSN -1
#define E_BAD_HDL -2

typedef std::function<void(int, std::string)> SocketHandler;

class DWSocketServer 
{
private:
	DWThreadPool *pool;
	int listenfd, epollfd;
	SocketHandler handler;

public:
	DWSocketServer(short port);

	~DWSocketServer();

	/**
	 * Set the handler for socket events.
	 */
	int setHandler(SocketHandler handler);

	/**
	 * Start listening at the given port.
	 * Any incoming message will be delivered
	 * to user through registered callback
	 * function.
	 *
	 * Return: 0 if success
	 *		   < 0 if error
	 */
	int startup();		

	int shutdown();

	/**
	 * Specially-designed read and write.
	 */
	int sread(int fd, std::string **sptr);
	int swrite(int fd, const std::string &str);

private:
	void multiEpoll();
	void sclose(int fd);
};
