//
// Created by gros on 14.12.17.
//

#ifndef VM_EXCEPTIONS_H
#define VM_EXCEPTIONS_H

#include <exception>
#include <string>


class ParserException: public std::exception
{
public:
  ParserException(std::string what) {
    this->what_ = what;
  }

  virtual const char* what() const throw()
  {
    return this->what_.c_str();
  }

private:
    std::string what_;
};

class VMRuntimeException: public std::exception
{
public:
    VMRuntimeException(std::string what) {
        this->what_ = what;
    }
    virtual const char* what() const throw()
    {
        return this->what_.c_str();
    }

private:
    std::string what_;
};


#endif //VM_EXCEPTIONS_H
