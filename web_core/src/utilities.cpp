/*
 *  Utilities.cpp
 *
 *  Created by Bill McIntire
 *
 *
 */


#include "../include/utilities.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <chrono>


//===================================================================================
// Sleep Unix/DOS Conversion


void Sleep(unsigned int milliSecs)
{
    usleep(milliSecs * 1000);
}

//===================================================================================
// Time

double GetDoubleTime()
{
    static auto start = std::chrono::steady_clock::now();
    auto time_now = std::chrono::steady_clock::now();
    double t = std::chrono::duration_cast<std::chrono::nanoseconds>(time_now - start).count();
    return(t * 1e-9);
}

char* TimeStamp()
{
	static char t[80];
	time_t z = time(NULL);
	strftime(t, 80, "%m/%d/%y %H:%M:%S", localtime(&z));
	return(t);
}


//===================================================================================
// String

void split(std::string str, std::string delimiter, std::vector<std::string> &str_copy)
{
    std::string token;
    std::size_t pos;
    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = str.substr(0, pos);
        str_copy.push_back(token);
        str.erase(0, pos + delimiter.length());
    }
}
