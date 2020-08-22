/*
 *  SimpleSocket.cpp
 *  Server
 *
 *  Created by Bill McIntire on Tue Mar 12 2002.
 *
 */

#include "../include/network_tools.h"
#include "../include/utilities.h"
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h> // added for linux


//======================================================================================
// TCP Server class that supports keep-alive and multiple connections
// Reference: http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/

void TCP_Server::Start(int network_port, int verbose)
{
	printf("TCP server starting\n");
	this->network_port = network_port;
	this->verbose = verbose;
	clientIndex = 0;
	runThread = std::thread([this]{ Run(); });
}

void TCP_Server::Stop()
{
    // disconnect clients using shutdown to release select call
    for (int i = 0; i < max_clients; i++) {
        if (client_socket[i] != 0) shutdown(client_socket[i], SHUT_RDWR);
    }

    // wait for run thread to stop
	running = 0;
	runThread.join();

    // close all sockets
    close(master_socket);
    close(new_socket);
    for (int i = 0; i < max_clients; i++) {
        if (client_socket[i] != 0) close(client_socket[i]);
    }

	printf("TCP server stopped (thread joined) %s\n", TimeStamp()); fflush(stdout);
}

int TCP_Server::mprintf(const char *format,...)
{
	va_list ap;
	int result;
	const int bufferSize = 1024;
	char s[bufferSize];

	va_start(ap, format);
	result = vsnprintf(s, bufferSize, format, ap);

	if (result >= bufferSize) { printf("*** buffer overflow in mprintf\n"); }

	msend(s, strlen(s));

	return(result);
}

void TCP_Server::msend(const char* data, size_t length)
{
	if (clientIndex > 0)	send(clientIndex , data , length, 0 );
}

typedef void (*funcP)();
TCP_Server::TCP_Server() {
}

TCP_Server::~TCP_Server()
{
	if (runThread.joinable()) {
		runThread.join();
		if (verbose) printf("TCP server thread joined in destructor. Calling Stop() first is recommended\n");
	}
}

int TCP_Server::Run()
{
	int opt = true;
	int addrlen=0, activity=0, i=0 , valread=0 , sd=0;
	int max_sd=0;
  struct sockaddr_in address;
  unsigned long last_client_address = 0;

	const int readBufferSize = 1024;

	char buffer[readBufferSize+1];  //data buffer

	//set of socket descriptors
	fd_set readfds;

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
        client_socket_time[i] = 0;
	}

	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( network_port );

	//bind the socket to localhost port
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (verbose) printf("Listener on port %d \n", network_port);

	//try to specify maximum of 8 pending connections for the master socket
	if (listen(master_socket, 8) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");

    // signal that we are now running
    running = 1;

    // do run loop
	while(running.load())
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		//add child sockets to set
		for ( i = 0 ; i < max_clients ; i++)
		{
			//socket descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);

			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
				max_sd = sd;
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

		if ((activity < 0) && (errno!=EINTR) && (errno!=EBADF))
		{
			perror("*** select error: \n");
			continue;
		}

		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				if (errno!=EBADF) perror("accept");
				//exit(EXIT_FAILURE);
                goto LoopEndLabel;
			}

            // inform user if this is a new client
            if (last_client_address != address.sin_addr.s_addr) {
                if (verbose) printf("A new client (%s) has connected to the server\n", inet_ntoa(address.sin_addr));
                last_client_address = address.sin_addr.s_addr;
            }

			//inform user of socket number - used in send and receive commands
			if (verbose) printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

			//add new socket to array of sockets
            clock_t minClk = clock();
            int oldestConnection = 0;
            int allSocketsInUse = 1;
			for (i = 0; i < max_clients; i++)
			{
				//if client position is empty add new socket
				if( client_socket[i] == 0 )
				{
                    allSocketsInUse = 0;
					client_socket[i] = new_socket;
                    client_socket_time[i] = clock();
					if (verbose) printf("Adding to list of sockets as %d\n" , i);

					break;
                }
			}

            // if all sockets are in use find the oldest socket, disconnect, and reuse it for new socket
            if (allSocketsInUse) {
                for (i = 0; i < max_clients; i++)
                {
                    if (client_socket_time[i] < minClk) {
                        minClk = client_socket_time[i];
                        oldestConnection = i;
                    }
                }
                printf("All %d server sockets in use.\nClosing oldest connection (i=%d, fd=%d) for reuse.\n", max_clients, oldestConnection, client_socket[oldestConnection]);
                close(client_socket[oldestConnection]);
                client_socket[oldestConnection] = new_socket;
                client_socket_time[oldestConnection] = clock();
                printf("Adding to list of sockets as %d\n" , oldestConnection);
            }
		}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];

			if (FD_ISSET( sd , &readfds))
			{
				//Check if it was for closing , and also read the incoming message
                valread = (int)recv(sd, buffer, readBufferSize, 0);
				if (valread <= 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
					if (verbose) printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
                    client_socket_time[i] = 0;
				}

				// Process the message that came in
				else
				{
					//set the string terminating NULL byte on the end of the data read
					buffer[valread] = '\0';

					// set port for response in msend()
					clientIndex = sd;
					receive(buffer, valread, i);
					clientIndex = 0;

                    // update activity time
                    client_socket_time[i] = clock();
				}
			}
		}
    LoopEndLabel:;
	}
	return 0;
}


//======================================================================================
// TCP client class

TCP_Client::TCP_Client() { sock_fd = 0; }

int TCP_Client::Connect(std::string ipAddress, int port)
{
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in address;
	address.sin_addr.s_addr = inet_addr(ipAddress.c_str()); /* assign the address */
	address.sin_port = htons(port);            				/* translate int2port num */
	address.sin_family = AF_INET;

	if (connect(sock_fd, (struct sockaddr *)&address, sizeof(address)) != 0)
	{
		sock_fd = 0;
		return 0;
	}

	return(1);
}

void TCP_Client::Close()
{
	if (sock_fd != 0) close(sock_fd);
}

int TCP_Client::mprintf(const char *format,...)
{
	va_list ap;
	int result;
	const int bufferSize = 1024;
	char s[bufferSize];

	va_start(ap, format);
	result = vsnprintf(s, bufferSize, format, ap);

	if (result >= bufferSize) { printf("*** buffer overflow in mprintf\n"); }

	result = msend(s, strlen(s));

	return(result);
}

int TCP_Client::msend(char* data, size_t length)
{
	ssize_t result = 0;
	result = send(sock_fd, data, length, 0);
	if (result < 0) printf("tcp client send error. result = %d\n", errno);
	return((int)result);
}

int TCP_Client::mrecv(char* data, size_t max_length)
{
	ssize_t result = 0;
	result = recv(sock_fd, data, max_length, 0);
	if (result == 0) printf("tcp client recv error. peer close the connection. result = %d\n", (int)result);
	if (result <  0) printf("tcp client recv error. result = %d\n", (int)result);
	return((int)result);
}

int TCP_Client::mpeak(char* data, size_t max_length)
{
	ssize_t result = 0;
	result = recv(sock_fd, data, max_length, MSG_PEEK);
	if (result == 0) printf("tcp client peak error. peer close the connection. result = %d\n", (int)result);
	if (result <  0) printf("tcp client peak error. result = %d\n", (int)result);
	return((int)result);
}
