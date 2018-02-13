//
// Created by gros on 18.01.18.
//

#include "Bytecode.h"

std::string vm_prolog(std::string blockName) {
    std::string prolog = R"END(
#include <iostream>
#include <vector>
#include <map>
#include <forward_list>

const unsigned char VAR = 0;
const unsigned char ARG = 1;
const unsigned char CONST = 2;
const unsigned char FUNC = 3;
const unsigned char THREAD = 4;

typedef void (*jit_func)(void*);
typedef std::string (*dtt_func)();


typedef struct dtt_arg {
    unsigned char type;
    int valInt;
    std::string valStr;
} dtt_arg;


class Function;

class FunctionPrototype {
public:
    FunctionPrototype(std::string, std::vector<dtt_func>*, std::forward_list<dtt_arg>*, int, std::map<std::string, int>);
    ~FunctionPrototype();
    Function* generate();

    std::string name;
    std::vector<dtt_func> *dtt;
    std::forward_list<dtt_arg> *dttArgs;
    int argTableSize;
    std::map<std::string, int> varTable;
    std::map<unsigned long, jit_func> *jit;
    unsigned long maxBlockSize;
};

class Function {
public:
    Function(FunctionPrototype&);
    ~Function();

    void run();
    jit_func compile();
    dtt_arg getNextArg(bool = true);
    void setArguments(std::vector<int>);
    std::vector<std::string> toStr() const;
    friend std::ostream& operator<<(std::ostream&, const Function&);

    std::string name;
    std::vector<dtt_func> *dtt;
    std::forward_list<dtt_arg> *dttArgs;
    std::forward_list<dtt_arg>::iterator dttArgsIt;
    int argTableSize;
    std::map<std::string, int> varTable;
    unsigned long vpc;

    std::map<unsigned long, jit_func> *jit;
    unsigned long *maxBlockSizePtr;

    bool anotherFunctionCalled;
    bool blocked;
    bool waiting;
    Function* returnFunction;
    std::string returnVariable;
};

class Thread {
public:
    Thread(std::string, Function*);
    ~Thread();
    void run();

    void joining(Thread*);
    void unblock();
    void receive(int);

    std::string name;
    Function* currentFunction;
    unsigned char status;
    std::vector<int> recvTable;
    std::vector<Thread*> joiningThreads;
    unsigned int priority;

    #if DEBUG == 1
    void refresh(WINDOW*, bool);
    bool currentColor;
    #endif
};

class VM {
public:
    void checkAllThreadsWaiting();

    Function* getCurrentFunction();
    Function* getNewFunction(std::string);

    Thread* getNewThread(std::string, std::string);
    Thread* getCurrentThread();
    void stopThread(std::string);
    Thread* getThread(std::string);

    bool changeScheduler(std::string);
    void setSchedulingFrequency(int);

    void print(std::string);

#if DEBUG == 1
    std::vector<WINDOW*> windows;
    WINDOW* terminal;
    void refresh();
#endif
};

)END";

    prolog += "extern \"C\" void ";
    prolog += blockName + "(VM &vm) {";
    prolog += R"END(
    auto currentFunction = vm.getCurrentFunction();
    dtt_arg arg0, arg1, arg2;
    int val0, val1, val2;
    Thread *thread;
    Function *function;
    std::vector<int> *recvTable;
    std::vector<int> newFunctionArgs;
    )END";
    return prolog;
}

std::string vm_epilog() {
    return R"END(
}
    )END";
}

std::string vm_assign(){
    return R"END(
    // vm_assign
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();

    val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable.at(arg1.valStr);

    currentFunction->varTable.at(arg0.valStr) = val1;
    currentFunction->vpc++;
    )END";
};
std::string vm_print() {
    return R"END(
    // vm_print
    arg0 = currentFunction->getNextArg();

    val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction->varTable.at(arg0.valStr);

    vm.print(std::to_string(val0));
    currentFunction->vpc++;
    )END";
}

std::string vm_call(){
    return R"END(
    // vm_call
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();

    currentFunction->anotherFunctionCalled = true;
    currentFunction->returnVariable = arg1.valStr;

    function = vm.getNewFunction(arg0.valStr);
    std::vector<int> functionArgs;

    for (int i = 0; i < function->argTableSize; ++i) {
        arg2 = currentFunction->getNextArg();
        val2 = arg2.valInt;
        if(arg2.type == VAR)
            val2 = currentFunction->varTable[arg2.valStr];
        functionArgs.push_back(val2);
    }

    function->setArguments(functionArgs);
    function->returnFunction = currentFunction;

    vm.getCurrentThread()->currentFunction = function;
    currentFunction->vpc++;
    return;
    )END";
};
std::string vm_return(){
    return R"END(
    // vm_return
    arg0 = currentFunction->getNextArg();

    val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction->varTable.at(arg0.valStr);

    if(currentFunction->returnFunction != nullptr) {
        currentFunction->returnFunction->varTable[currentFunction->returnFunction->returnVariable] = val0;
        currentFunction->returnFunction->anotherFunctionCalled = false;
    }
    vm.getCurrentThread()->currentFunction = currentFunction->returnFunction;
    delete currentFunction;
    )END";
};

std::string vm_send(){
    return R"END(
    // vm_send
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();

    val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable.at(arg1.valStr);

    thread = vm.getThread(arg0.valStr);
    if(thread != nullptr)
        thread->receive(val1);
    currentFunction->vpc++;
    )END";
};
std::string vm_recv(){
    return R"END(
    // vm_recv
    arg0 = currentFunction->getNextArg(false);

    recvTable = &vm.getCurrentThread()->recvTable;
    if(recvTable->size() != 0) {
        currentFunction->vpc++;
        currentFunction->getNextArg();

        currentFunction->varTable.at(arg0.valStr) = recvTable->back();
        recvTable->pop_back();

        currentFunction->waiting = false;
    } else {
        vm.checkAllThreadsWaiting();
        currentFunction->waiting = true;
        return;
    }
    )END";
};
std::string vm_start(){
    return R"END(
    // vm_start
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();

    thread = vm.getNewThread(arg1.valStr, arg0.valStr);

    for (int i = 0; i < thread->currentFunction->argTableSize; ++i) {
        arg2 = currentFunction->getNextArg();
        val2 = arg2.valInt;
        if(arg2.type == VAR)
            val2 = currentFunction->varTable[arg2.valStr];
        newFunctionArgs.push_back(val2);
    }

    thread->currentFunction->setArguments(newFunctionArgs);
    newFunctionArgs.clear();
    currentFunction->vpc++;
    )END";
};
std::string vm_join(){
    return R"END(
    // vm_join
    arg0 = currentFunction->getNextArg(false);

    thread = vm.getThread(arg0.valStr);
    if(thread != nullptr) {
        currentFunction->blocked = true;
        thread->joining(vm.getCurrentThread());
        return;
    } else {
        currentFunction->blocked = false;
        currentFunction->vpc++;
        currentFunction->getNextArg();
    }
    )END";
};
std::string vm_stop(){
    return R"END(
    // vm_stop
    arg0 = currentFunction->getNextArg();

    vm.stopThread(arg0.valStr);
    currentFunction->vpc++;
    )END";
};
std::string vm_priority(){
    return R"END(
    // vm_priority
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();

    thread = vm.getThread(arg0.valStr);
    if(thread != nullptr && arg1.valInt >= 1 && arg1.valInt <= 10)
        thread->priority = arg1.valInt;
    currentFunction->vpc++;
    )END";
};

std::string vm_add() {
    return R"END(
    // vm_add
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();
    arg2 = currentFunction->getNextArg();

    val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable.at(arg1.valStr);

    val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable.at(arg2.valStr);

    currentFunction->varTable.at(arg0.valStr) = val1 + val2;
    currentFunction->vpc++;
    )END";
}
std::string vm_sub() {
    return R"END(
    // vm_sub
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();
    arg2 = currentFunction->getNextArg();

    val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable.at(arg1.valStr);

    val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable.at(arg2.valStr);

    currentFunction->varTable.at(arg0.valStr) = val1 - val2;
    currentFunction->vpc++;
    )END";
};
std::string vm_div() {
    return R"END(
    // vm_div
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();
    arg2 = currentFunction->getNextArg();

    val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable.at(arg1.valStr);

    val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable.at(arg2.valStr);

    currentFunction->varTable.at(arg0.valStr) = val1 / val2;
    currentFunction->vpc++;
    )END";
};
std::string vm_mul() {
    return R"END(
    // vm_mul
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();
    arg2 = currentFunction->getNextArg();

    val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable.at(arg1.valStr);

    val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable.at(arg2.valStr);

    currentFunction->varTable.at(arg0.valStr) = val1 * val2;
    currentFunction->vpc++;
    )END";
};

