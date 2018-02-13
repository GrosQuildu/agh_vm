//
// Created by gros on 13.12.17.
//


#if DEBUG == 1
#include <ncurses.h>
#endif

#include "VM.h"
#include <unistd.h>


void VM::initialize(const std::string codeDirPath, const std::string blocksDir,
                    const std::string defaultScheduler, bool rebuild) {
    if(!VM::isInitialized) {
        this->functionFactory = new FunctionFactory;
        this->threadManager = new ThreadManager;
        this->threadSchedulers = new std::vector<ThreadScheduler*>;

        VM::functionFactory->initialize(codeDirPath);

        VM::threadSchedulers->push_back(new RoundRobinScheduler());
        VM::threadSchedulers->push_back(new FIFOScheduler());
        VM::threadSchedulers->push_back(new PriorityScheduler());

        auto schedulerSet = VM::changeScheduler(defaultScheduler);
        if(!schedulerSet)
            throw VMRuntimeException("Scheduler " + defaultScheduler + " not found");

        this->rebuild = rebuild;
        this->blocksDir = blocksDir;

        VM::isInitialized = true;

        #if DEBUG == 1
        initscr();
        cbreak();
        nodelay(stdscr, TRUE);
        keypad(stdscr, FALSE);
        noecho();
        curs_set(0);

        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_BLUE, COLOR_BLACK);
        init_pair(6, COLOR_YELLOW, COLOR_BLACK);
        init_pair(7, COLOR_RED, COLOR_BLACK);

        wbkgd(stdscr, COLOR_PAIR(1));
        bkgd(COLOR_PAIR(1));

        int terminalHeight = 10;
        this->threadWinWidth = 35;
        this->threadWinHeight = LINES - terminalHeight;
        this->threadWinMargin = 2;

        int windowsAmount = COLS / (VM::threadWinWidth + VM::threadWinMargin);
        if (windowsAmount * (VM::threadWinWidth + VM::threadWinMargin) + VM::threadWinMargin > COLS)
            windowsAmount--;
        for (int i = 0; i < windowsAmount; i++)
            VM::windows.push_back(newwin(VM::threadWinHeight, VM::threadWinWidth, VM::threadWinMargin,
                                             (VM::threadWinWidth + VM::threadWinMargin) * i + VM::threadWinMargin));

        VM::terminal = newwin(LINES - VM::threadWinHeight - VM::threadWinMargin, COLS - 2*VM::threadWinMargin,
                              VM::threadWinHeight + VM::threadWinMargin, VM::threadWinMargin);
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
    if(!VM::functionFactory->haveFunction("MAIN")) {
        throw VMRuntimeException("MAIN function not found");
    }
    auto mainFunction = VM::functionFactory->makeFunction("MAIN");
    VM::threadManager->addThread("MAIN", mainFunction);

    VM::threadManager->schedule();
}

void VM::stop() {
    VM::threadManager->clearAll();
    delete this->functionFactory;
    delete this->threadManager;
    delete this->threadSchedulers;
    this->functionFactory = nullptr;
    this->threadManager = nullptr;
    this->threadSchedulers = nullptr;
    #if DEBUG == 1
    endwin();
    #endif
    std::cout<<"Virtual Machines stopped\n";
}

Function* VM::getCurrentFunction() {
    #if DEBUG == 1
    VM::refresh();
    #endif
    return VM::threadManager->getCurrentFunction();
}

Thread* VM::getCurrentThread() {
    return VM::threadManager->getCurrentThread();
}

Function* VM::getNewFunction(std::string functionName) {
    return VM::functionFactory->makeFunction(functionName);
}

Thread* VM::getNewThread(std::string threadName, std::string functionName) {
    auto newFunction = VM::functionFactory->makeFunction(functionName);
    return VM::threadManager->addThread(threadName, newFunction);
}

void VM::stopThread(std::string threadName) {
    try {
        VM::threadManager->removeThread(threadName);
    } catch(ThreadManagerException e) {
        VM::stop();
    }
}

bool VM::changeScheduler(std::string schedulerName) {
    for(auto it = VM::threadSchedulers->begin(); it != VM::threadSchedulers->end(); it++) {
        if((*it)->getName() == schedulerName) {
            VM::threadManager->changeScheduler(*it);
            return true;
        }
    }
    return false;
}

void VM::setSchedulingFrequency(unsigned long frequency) {
    VM::functionFactory->setSchedulingFrequency(frequency);
}

void VM::print(std::string value) {
    #if DEBUG == 1
    wclear(VM::terminal);
    box(VM::terminal, 0 , 0);
    mvwaddstr(VM::terminal, 1, 1, "TERMINAL:");

    mvwaddstr(VM::terminal, 3, 1, value.c_str());
    wrefresh(VM::terminal);
//    usleep(4 * 100000);
    #else
    std::cout<<value<<std::endl;
    #endif
}

Thread* VM::getThread(std::string threadName) {
    return this->threadManager->getThread(threadName);
}

void VM::checkAllThreadsWaiting() {
    VM::threadManager->checkAllThreadsWaiting();
}


#if DEBUG == 1
void VM::refresh() {
    VM::threadManager->refreshThreads(VM::windows, 0);
    usleep(5 * 100000);
}
#endif
