//
// Created by gros on 13.12.17.
//


#include "VM.h"


bool VM::isInitialized = false;
FunctionFactory VM::functionFactory;
ThreadManager VM::threadManager;


void VM::initialize(const std::string& codeDirPath) {
    if(!VM::isInitialized) {
        VM::functionFactory.initialize(codeDirPath);
        VM::isInitialized = true;
    }
}

VM& VM::getVM() {
    static VM vm;
    return vm;
}

void VM::start() {
    if(!VM::functionFactory.haveFunction("MAIN")) {
        throw VMRuntimeException("MAIN function not found");
    }
    auto mainFunction = VM::functionFactory.makeFunction("MAIN");
    auto mainThread = Thread("MAIN", mainFunction);
    VM::threadManager.add(mainThread);

    VM::threadManager.schedule();
}

Function* VM::getCurrentFunction() {
    return VM::threadManager.getCurrentFunction();
}

Thread* VM::getCurrentThread() {
    return VM::threadManager.getCurrentThread();
}
