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
std::vector<ThreadScheduler*> VM::threadSchedulers;

int VM::ThreadWinWidth = 30;
int VM::ThreadWinHeight = 35;
int VM::ThreadWinMargin = 2;

#if DEBUG == 1
std::vector<WINDOW*> VM::windows;
WINDOW* VM::terminal;
#endif

void VM::initialize(const std::string codeDirPath, const std::string defaultScheduler) {
    if(!VM::isInitialized) {
        VM::functionFactory.initialize(codeDirPath);

        VM::threadSchedulers.push_back(new RoundRobinScheduler());
        VM::threadSchedulers.push_back(new FIFOScheduler());
        auto schedulerSet = VM::changeScheduler(defaultScheduler);
        if(!schedulerSet)
            throw VMRuntimeException("Scheduler " + defaultScheduler + " not found");

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

        VM::terminal = newwin(LINES - VM::ThreadWinHeight - VM::ThreadWinMargin, COLS - 2*VM::ThreadWinMargin,
                              VM::ThreadWinHeight + VM::ThreadWinMargin, VM::ThreadWinMargin);
        box(VM::terminal, 0 , 0);
        mvwaddstr(VM::terminal, 1, 1, "TERMINAL:");
        wrefresh(VM::terminal);
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
    for(auto it = VM::threadSchedulers.begin(); it != VM::threadSchedulers.end(); it++)
        delete *it;
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

bool VM::changeScheduler(std::string schedulerName) {
    for(auto it = VM::threadSchedulers.begin(); it != VM::threadSchedulers.end(); it++) {
        if((*it)->getName() == schedulerName) {
            VM::threadManager.changeScheduler(*it);
            return true;
        }
    }
    return false;
}

void VM::setMaxBlockSize(unsigned long maxBlockSize) {
    VM::functionFactory.setMaxBlockSize(maxBlockSize);
}

void VM::print(std::string value) {
    #if DEBUG == 1
    mvwaddstr(VM::terminal, 3, 1, "");
    mvwaddstr(VM::terminal, 3, 1, value.c_str());
    wrefresh(VM::terminal);
    #else
    std::cout<<value<<std::endl;
    #endif
}

#if DEBUG == 1
void VM::refresh() {
    VM::threadManager.refreshThreads(VM::windows, 0);
    usleep(1 * 1000000);
}
#endif
