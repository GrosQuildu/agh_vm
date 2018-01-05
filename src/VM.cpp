//
// Created by gros on 13.12.17.
//


#if DEBUG == 1
#include <ncurses.h>
#endif

#include "VM.h"
#include <unistd.h>


bool VM::isInitialized = false;
FunctionFactory VM::functionFactory;
ThreadManager VM::threadManager;


int VM::ThreadWinWidth = 50;
int VM::ThreadWinHeight = 50;
int VM::ThreadWinMargin = 2;

#if DEBUG == 1
std::vector<WINDOW*> VM::windows;
#endif

void VM::initialize(const std::string& codeDirPath) {
    if(!VM::isInitialized) {
        VM::functionFactory.initialize(codeDirPath);
        auto defaultScheduler = new RoundRobinScheduler();
        VM::threadManager.changeScheduler(defaultScheduler);
        VM::isInitialized = true;

        #if DEBUG == 1
        initscr();
        cbreak();
        nodelay(stdscr, TRUE);
        keypad(stdscr, FALSE);
        noecho();

        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_BLUE, COLOR_BLACK);
        init_pair(6, COLOR_YELLOW, COLOR_BLACK);
        init_pair(7, COLOR_RED, COLOR_BLACK);


        int windowsAmount = COLS / (VM::ThreadWinWidth + VM::ThreadWinMargin);
        if (windowsAmount * (VM::ThreadWinWidth + VM::ThreadWinMargin) + VM::ThreadWinMargin > COLS)
            windowsAmount--;
        for (int i = 0; i < windowsAmount; i++)
            VM::windows.push_back(newwin(VM::ThreadWinHeight, VM::ThreadWinWidth, VM::ThreadWinMargin,
                                             (VM::ThreadWinWidth + VM::ThreadWinMargin) * i + VM::ThreadWinMargin));
        #endif
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
    #if DEBUG == 1
    VM::refresh();
    #endif
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

#if DEBUG == 1
void VM::refresh() {
    VM::threadManager.refreshThreads(VM::windows, 0);
    usleep(1 * 1000000);
}
#endif

void VM::setSchedulingFrequency(int frequency) {
    VM::functionFactory.setSchedulingFrequency(frequency);
}
