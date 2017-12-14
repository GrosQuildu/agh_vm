//
// Created by gros on 13.12.17.
//

#include "VM.h"


VM::VM(const std::string & codePath) {
    if(!isInitialized) {
        isInitialized = true;
        FunctionFactory::addFunction(codePath);
    }
}

void VM::start() {
    auto mainFunction = FunctionFactory::makeFunction("MAIN");
    VM::getVM().currentFunction_ = mainFunction;
    mainFunction->run();
}

Function* VM::getCurrentFunction() {
    return VM::currentFunction_;
}
