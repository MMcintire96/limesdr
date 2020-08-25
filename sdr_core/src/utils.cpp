#include "utils.h"
#include <unistd.h>


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

//modifes string !!!!
std::string getKey(std::string &str, std::string delimiter) {
  std::string key;
  std::size_t pos;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    key = str.substr(0, pos);
    str.erase(0, pos + delimiter.length());
  }
  return key;
}
