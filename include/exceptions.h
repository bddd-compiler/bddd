#ifndef BDDD_EXCEPTIONS_H
#define BDDD_EXCEPTIONS_H

#include <exception>
#include <string>
#include <utility>

class MyException : public std::exception {
private:
  std::string msg_;

public:
  explicit MyException(std::string msg) : msg_(std::move(msg)) {}
  std::string Msg() { return msg_; }

  MyException copy() { return MyException(msg_); }
};

#endif  // BDDD_EXCEPTIONS_H
