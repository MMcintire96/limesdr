
/*
 *  Utilities.h
 *
 *  Created by Bill McIntire
 *
 *
 */

#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <vector>



//===================================================================================
// Sleep - in milliseconds

void Sleep(unsigned int milliSecs);


//===================================================================================
//	Time

double GetDoubleTime();
char* TimeStamp();
void split(std::string str, std::string delimiter, std::vector<std::string> &str_copy);


#endif	// End of Utilities.h file
