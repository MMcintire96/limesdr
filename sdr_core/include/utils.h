#ifndef UTILITIES_H
#define UTILITIES_H
#include <iostream>
#include <vector>

void split(std::string str, std::string delimiter, std::vector<std::string> &str_copy);
std::string getKey(std::string &str, std::string delimiter);

#endif	// End of Utilities.h file
