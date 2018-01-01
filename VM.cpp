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
    VM::threadManager.addThread("MAIN", mainFunction);

    VM::threadManager.schedule();
}

void VM::stop() {
    VM::threadManager.clearAll();
    std::cout<<"Virtual Machines stopped\n";
}

Function* VM::getCurrentFunction() {
    return VM::threadManager.getCurrentFunction();
}

Thread* VM::getCurrentThread() {
    return VM::threadManager.getCurrentThread();
}

Function* VM::getNewFunction(std::string functionName) {
    return VM::functionFactory.makeFunction(functionName);
}

Thread* VM::getNewThread(std::string threadName, std::string functionName) {
    auto mainFunction = VM::functionFactory.makeFunction(functionName);
    return VM::threadManager.addThread(threadName, mainFunction);
}

void VM::stopThread(std::string threadName) {
    try {
        VM::threadManager.removeThread(threadName);
    } catch(ThreadManagerException e) {
        VM::stop();
    }
}
