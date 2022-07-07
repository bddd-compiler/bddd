#ifndef BDDD_EXCEPTIONS_H
#define BDDD_EXCEPTIONS_H

#include <exception>
#include <string>
#include <utility>

class MyException : public std::exception {
private:
  std::string msg;

public:
  explicit MyException(std::string msg) : msg(std::move(msg)) {}
  std::string getMsg() { return msg; }

  MyException copy() { return MyException(msg); }
};

#endif  // BDDD_EXCEPTIONS_H
