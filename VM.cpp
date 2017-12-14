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


VM& VM::getVM(const std::string& codePath) {
    if(!VM::isInitialized) {
        VM::functionFactory.initialize(codePath);
        VM::isInitialized = true;
    }
    static VM vm;
    return vm;
}

VM& VM::getVM() {
    return VM::getVM("");
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
