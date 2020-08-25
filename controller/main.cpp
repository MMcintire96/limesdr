//
//  main.cpp
//  Ideas
//
//  Created by bill on 8/23/20.
//  Copyright © 2020 Bill McIntire. All rights reserved.
//

#include <iostream>
#include <unistd.h>

#include "Controller.h"


int main(int argc, const char * argv[])
{
    Controller controller;
    
    controller.Start();
    
    for (int i=0; i<100; i++) {

        sleep(1);
        
        std::string data = "";
        controller.ReadPipe(&data);
        printf("%s", data.c_str());

        controller.WritePipe("abc");
    }
    printf("\n");
    controller.Stop();
    
    
    return 0;
}
