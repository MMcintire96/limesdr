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

#include "network_tools.h"
#include "utilities.h"
#include <vector>
#include <map>
#include <sstream>
#include <mutex>


class Worker
{
public:
    virtual void Process(std::string message, std::map<std::string, std::string> request) = 0;
};

class MyWorker : public Worker
{
public:
    void Process(std::string message, std::map<std::string, std::string> request)
    {
        printf("%s\n", message.c_str());
    }
};


// MyServer is derived from the TCP_Server class.
// It overrides the one virtual function in the TCP_Server class.
// The TCP_Server class will call this function whenever it receives data from a client.
// MyServer should process the data and respond (optional) to the client using msend() and mprintf().
// The receive function is always called by the server thread.
// The variable 'serverIsUp' is used by the main thread to track the status of the server.
// 'lastClientMessage' is used to pass the data received to the main thread.
// A mutex is use to prevent simultanous access by the two threads.

class MyServer : public TCP_Server
{
public:

  void receive(char* buffer, int bufferSize, int clientIndex)
  {
    myServerMutex.lock();
    printf("\nReceived: \n%s\n", buffer);

    std::string message = buffer;
    lastClientMessage = buffer;

    std::map<std::string, std::string> request;
    std::map<std::string, Worker*> routeTable;

    MyWorker myWorker;
    routeTable["path"] = &myWorker;

    message.erase(message.find_last_of("\n"), 1);

    //this is illegal on a full implementation that a quit string kills the server
    if (message == "quit") serverIsUp = 0;

    //probably breaks, only HTTP 1
    if (message.find("HTTP") >= 0)  {
      parseHTTP(message, request);
      router(routeTable, request, message);
    }


    // return header --- this is just a place holder to keep the browser happy
      mprintf("HTTP/1.0 200 OK\r\n");
      mprintf("Connection: keep-alive\r\n");

      mprintf("Content-Type: text/html; charset=ISO-8859-1\r\n");

      int byteCnt = 29;
      mprintf("Content-Length: %d\r\n", byteCnt);
      mprintf("\r\n");
      mprintf("Thanks for visiting our page.");

    myServerMutex.unlock();
  }

  //all Views should really be defined as classes for a large scale implementation
  //for now we can make views just check request["method"] and handle based on this
  //this will get messy for really complex views
  //this should return headers(200, OK) and an html File if GET,
  //this should return headers(200, OK) and json::data if POST,PUT
  //this should return headers(200, OK) for all other methods
  void home(std::string message, std::map<std::string, std::string> request) {
    std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-length: 20\r\n\r\n<h1>Hello World</h1>";
    msend(resp.c_str(), strlen(resp.c_str())); //seg faults? probably has to do with buffer size and what not
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
  void router(std::map<std::string, Worker*> &routeTable, std::map<std::string, std::string> &request, std::string message)
  {
    //sig_ptr func = routeTable[request["path"]];
    //(this->*func)(message, request); //not a true implementation of currying but works for routing
    //  routeTable[request["path"]];
      routeTable.find("path")->second->Process(message, request);
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
