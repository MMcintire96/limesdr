//
//  Controller.cpp
//  Ideas
//
//  Created by bill on 8/23/20.
//  Copyright © 2020 Bill McIntire. All rights reserved.
//

#include "Controller.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void Controller::Start()
{
    // open the pipe
    write_fifo = "/tmp/limesdr-control-fifo-write";
    int r = mkfifo(write_fifo.c_str(), 0666);
    if (r < 0) {
        //printf("*** Failed to open the write pipe in Controller::Start()\n");
        //perror("error message:");
        //exit(1);
    }

    // start the thread running
    threadRun = 1;
    thread = std::thread([this]{ ThreadLoop(); });
}

void Controller::Stop()
{
    // stop the thread
    threadRun = 0;
    thread.join();
    
    // close the pipe
    // ... needs to be finished
}

void Controller::Register()
{
    
}

void Controller::WritePipe(std::string writeData)
{
    write_pipe = open(write_fifo.c_str(), O_CREAT | O_WRONLY);
    if (write_pipe < 0) {
        printf("*** Failed to open the write pipe in Controller::WritePipe()\n");
        exit(1);
    }
    write(write_pipe, writeData.c_str(), writeData.size());
    close(write_pipe);
}

void Controller::ReadPipe(std::string* readData)
{
    // lock the mutex and read the data in the output buffer
    mutex.lock();
    *readData = readBuffer;
    readBuffer = "";
    mutex.unlock();
}

void Controller::ThreadLoop()
{
    // open the pipe
    std::string fifo = "/tmp/limesdr-control-fifo-read";
    int r = mkfifo(fifo.c_str(), 0666);
    if (r < 0) {
        //printf("*** Failed to open the read pipe in Controller::Start()\n");
        //exit(1);
    }

    while (threadRun.load())
    {
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
