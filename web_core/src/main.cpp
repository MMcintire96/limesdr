//
//  main.cpp
//  SimpleServer
//
//  Created by bill on 8/11/20.
//
//

// Test with netcat.
// Start this server program running.
// Then open a terminal window and type
//   nc 127.0.0.1 8089
// Afer nc starts, you can type anything and it will echo back
// It will also be printed in the terminal of the server
// You can also connect with a browser
//   http://127.0.0.1:8089/?Jackisadog
// It will print the browser headers and so on
// The browser will hang since this program does not send a proper response
// A simple improvement would be to return a header and an HTML file
//

#include "../include/network_tools.h"
#include "../include/utilities.h"
#include <vector>
#include <map>
#include <sstream>
#include <mutex>


class Worker {
public:
    virtual void Process(std::string message, std::map<std::string, std::string> request) = 0;
};

class MyWorker : public Worker {
public:
    void Process(std::string message, std::map<std::string, std::string> request) {
      printf("%s\n", message.c_str());
    }
};

typedef void (*funcP)();
class RouteTable {
  public:
    std::map<std::string, funcP> table;
};

void home() {
  //mprintf("HTTP/1.0 200 OK\r\n");
  //mprintf("Connection: keep-alive\r\n");
  //mprintf("Content-Type: text/html; charset=ISO-8859-1\r\n");
  //int byteCnt = 29;
  //mprintf("Content-Length: %d\r\n", byteCnt);
  //mprintf("\r\n");
  //mprintf("Thanks for visiting our page.");
}


class MyServer : public TCP_Server {

public:

  void receive(char* buffer, int bufferSize, int clientIndex)
  {
    myServerMutex.lock();
    printf("\nReceived: \n%s\n", buffer);

    std::string message = buffer;
    lastClientMessage = buffer;

    RouteTable t;
    t.table["/"] = &home;

    std::map<std::string, std::string> request;
    //std::map<std::string, Worker*> routeTable;

    //MyWorker myWorker;
    //routeTable["path"] = &myWorker;

    message.erase(message.find_last_of("\n"), 1);

    //this is illegal on a full implementation that a quit string kills the server
    if (message == "quit") serverIsUp = 0;

    //probably breaks, only HTTP 1
    if (message.find("HTTP") >= 0)  {
      parseHTTP(message, request);
      router(t, request, message);
    }

    myServerMutex.unlock();
  }



  // parse http headers into a map
  // should split path on first ? and create a nested queryParams map based on ? and &
  // http is network protocol abstraction and shoud be placed above the tcp layer
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

  // handle routeTable mapping for multiple /URL using function pointers
  // the route table should be above the http layer
  // if (!map[string]) this shoudl return generic 404 page
  //void router(std::map<std::string, Worker*> &routeTable, std::map<std::string, std::string> &request, std::string message) {
  void router(RouteTable &t, std::map<std::string, std::string> &request, std::string message) {
    funcP p = t.table[request["path"]];
    if (p != NULL)
      p();
    printf("Routing to function at %p\n", p);
  }


  int serverIsUp = 1;
  std::mutex myServerMutex;
  std::string lastClientMessage;
};


// create an instance of the MyServer class
MyServer tcp_Server;
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
