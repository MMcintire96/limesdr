//
//  Controller.hpp
//  Ideas
//
//  Created by bill on 8/23/20.
//  Copyright © 2020 Bill McIntire. All rights reserved.
//

#ifndef Controller_hpp
#define Controller_hpp

#include <stdio.h>
#include <thread>
#include <mutex>
#include <atomic>

class Controller
{
public:
    // Start and Stop must be called by the same thread.
    void Start();
    void Stop();
    
    void Register();
    
    void WritePipe(std::string writeData);
    
    void ReadPipe(std::string* readData);
    
private:
    
    void ThreadLoop();
    
    std::mutex mutex;
    std::thread thread;
    std::atomic<int8_t> threadRun;
    
    int read_pipe = 0;
    int write_pipe = 0;

    std::string write_fifo;
    
    std::string readBuffer;
};


#endif /* Controller_hpp */
