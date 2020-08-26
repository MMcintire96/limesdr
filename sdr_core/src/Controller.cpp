//
//  Controller.cpp
//
//
//  Created by bill on 8/23/20.
//
//

#include "Controller.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstring> //memset linux

void Controller::start() {
    
  // open the pipe
  write_fifo = "/tmp/lsdr-status-fifo";
  int r = mkfifo(write_fifo.c_str(), 0666);
  if (r < 0) {
      //printf("*** Failed to open the write pipe in Controller::Start()\n");
      //perror("error message:");
  }

  // start the thread running
  threadRun = 1;
  thread = std::thread([this]{ threadLoop(); });
}

void Controller::stop() {
  // stop the thread
  threadRun = 0;
  thread.join();

  // close the pipe
  // ... needs to be finished
}

void Controller::connect() {}

void Controller::writePipe(std::string writeData) {
  write_pipe = open(write_fifo.c_str(), O_CREAT | O_WRONLY);
  if (write_pipe < 0) {
      printf("*** Failed to open the write pipe in Controller::WritePipe()\n");
      exit(1);
  }
  write(write_pipe, writeData.c_str(), writeData.size());
  close(write_pipe);
}

void Controller::readPipe(std::string* readData) {
    // lock the mutex and read the data in the output buffer
    mutex.lock();
    *readData = readBuffer;
    readBuffer = "";
    mutex.unlock();
}

void Controller::threadLoop() {
  // open the pipe
  std::string fifo = "/tmp/lsdr-command-fifo";
  int r = mkfifo(fifo.c_str(), 0666);
  if (r < 0) {
      //printf("*** Failed to open the read pipe in Controller::Start()\n");
      //exit(1);
  }

  while (threadRun.load()) {
    // block here waiting for data
    char temp[256];
    memset(temp, 0, 256);


    read_pipe = open(fifo.c_str(), O_CREAT | O_RDONLY);
    if (read_pipe < 0) {
        printf("*** Failed to open the read pipe in Controller::Start()\n");
        exit(1);
    }

    read(read_pipe, temp, 256);

    close(read_pipe);
    read_pipe = 0;

    // lock the mutex and move the data to the output buffer
    mutex.lock();
    readBuffer += temp;
    //printf("%s ", readBuffer.c_str());

    //sleep(1);
    mutex.unlock();
  }


}
