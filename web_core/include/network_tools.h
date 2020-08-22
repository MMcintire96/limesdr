#ifndef	NETWORK_TOOLS03122002
#define NETWORK_TOOLS03122002

/*
 *
 *  Created by Bill McIntire on Tue Mar 12 2002.
 *
 */

#include <string>
#include <cstring>
#include <thread>
#include <atomic>


//======================================================================================
// TCP Server class that supports keep-alive and multiple connections
// Reference: http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/

class TCP_Server
{
public:
	int clientIndex;
    static const int max_clients = 16;

public:

	void Start(int network_port, int verbose);
	void Stop();
	TCP_Server();
	~TCP_Server();

	void SetVerbose(int verbose) { this->verbose = verbose; }
	int mprintf(const char *format,...);

protected:

	virtual void receive(char* buffer, int bufferSize, int clientIndex)=0;

	void msend(const char* data, size_t length);

private:
	int verbose;
	int network_port;
	std::thread runThread;
    std::atomic<int> running;

    clock_t client_socket_time[max_clients];
	int client_socket[max_clients];
	int master_socket, new_socket;

	int Run();
};


//======================================================================================
// TCP Client class that supports single connection to a server

class TCP_Client
{
public:
	int sock_fd;

public:

	TCP_Client();

	int  Connect(std::string ipAddress, int port);
	void Close();

	int mprintf(const char *format,...);
	int msend(char* data, size_t length);
	int mrecv(char* data, size_t max_length);
	int mpeak(char* data, size_t max_length);
};

#endif
