//
//  main.cpp
//  SimpleServer
//
//  Created by bill on 8/11/20.
//
//

#include "../include/network_tools.h"
#include "../include/utilities.h"
#include <vector>
#include <map>
#include <sstream>
#include <functional>
#include <mutex>


using fp = void(*)();
std::map<std::string, fp> routeTable;


void home() {
  printf("here");
  tcp_Server.mprintf("HTTP/1.0 200 OK\r\n");
  tcp_Server.mprintf("Connection: keep-alive\r\n");
  tcp_Server.mprintf("Content-Type: text/html; charset=ISO-8859-1\r\n");
  int byteCnt = 29;
  tcp_Server.mprintf("Content-Length: %d\r\n", byteCnt);
  tcp_Server.mprintf("\r\n");
  tcp_Server.mprintf("Thanks for visiting our page.");
}

class Server : public TCP_Server {

public:

  void receive(char* buffer, int bufferSize, int clientIndex) {
    myServerMutex.lock();
    printf("\nReceived: \n%s\n", buffer);

    std::string message = buffer;
    lastClientMessage = buffer;


    std::map<std::string, std::string> request;

    message.erase(message.find_last_of("\n"), 1);

    //this is illegal on a full implementation that a quit string kills the server
    if (message == "quit") serverIsUp = 0;

    //probably breaks, only HTTP 1
    if (message.find("HTTP") >= 0)  {
      parseHTTP(message, request);
      router(routeTable, request, message);
    }

    myServerMutex.unlock();
  }


  void buildRouteTable() {
    routeTable["/"] = &home;
  }


  void parseHTTP(std::string message, std::map<std::string, std::string> &request) {

    request["method"] = message.substr(0, message.find(" "));
    request["path"] = message.substr(message.find("/"), message.find("HTTP")-5);

    std::vector<std::string> line_vect;
    split(message, "\n", line_vect);
    for (int i=0; i<line_vect.size(); i++) {
      std::vector<std::string> line;
      split(line_vect.at(i), ":", line);
      if (line.size() == 2) {
        request[line[0]] = line[1];
      }
    }
    if (message.find("\r\n\n") == -1) return;
  }

  void router(std::map<std::string, fp> &routeTable, std::map<std::string, std::string> &request, std::string message) {
    fp p = routeTable[request["path"]];
    p();
    printf("Routing to function at %p\n", &p);
  }


  int serverIsUp = 1;
  std::mutex myServerMutex;
  std::string lastClientMessage;
};




// create an instance of the MyServer class
Server tcp_Server;

// Main thread
int main(int argc, const char * argv[])
{
  // set up network port whhere the server will listen for connections
  int network_port = 8089;

  // set verbose = 1 if you want to see information about client connections
  int verbose = 0;

 // launch the server thread from the main thread (this thread)
  tcp_Server.Start(network_port, verbose);

  // inform the main thread that the server has been started
  tcp_Server.serverIsUp = 0;


  // loop the main thread
  // just sleeping and counting the bytes received
  // a mutex is used to ensure this thread does not access 'lastClientMessage' when the server thread is accessing it
  int byteCount = 0;
  while (tcp_Server.serverIsUp) {
    Sleep(500);
    tcp_Server.myServerMutex.lock();
    byteCount += tcp_Server.lastClientMessage.size();
    tcp_Server.lastClientMessage = "";
    tcp_Server.myServerMutex.unlock();
  }

  // stop the server thread (only call stop from the thread that started the server)
  // this will block until the server thread joins the main thread
  tcp_Server.Stop();

  printf("byteCount = %d\n", byteCount);

  return 0;
}
