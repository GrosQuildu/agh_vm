//
// Created by gros on 13.12.17.
//

#include "VM.h"
#include "Exceptions.h"


//VM::VM(const std::string & codePath) {
//
//}

bool VM::isInitialized = false;
FunctionFactory VM::functionFactory;


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
    VM::getVM().currentFunction_ = mainFunction;
    mainFunction->run();
}

Function* VM::getCurrentFunction() {
    return VM::currentFunction_;
}
