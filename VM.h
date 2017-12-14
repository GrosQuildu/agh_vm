//
// Created by gros on 13.12.17.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <set>
#include "Function.h"

using std::set;


class VM {
public:
    static VM& getVM(const std::string& codePath);
    static VM& getVM();

    void start();
    Function* getCurrentFunction();
    Function *currentFunction_;

private:
//    VM(const std::string& codePath);
//    VM(const VM & );
    static bool isInitialized;
    static FunctionFactory functionFactory;
};


#endif //VM_VM_H
