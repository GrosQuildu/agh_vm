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
    static VM& getVM(const std::string& codePath)
    {
        static VM vm(codePath);
        return vm;
    }

    static VM& getVM()
    {
        return VM::getVM("");
    }

    void start();
    Function* getCurrentFunction();
    Function *currentFunction_;

private:
    VM(const std::string&);
    VM(const VM & );
    bool isInitialized = false;
};


#endif //VM_VM_H
