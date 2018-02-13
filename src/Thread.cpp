//
// Created by gros on 13.12.17.
//

#include "Thread.h"
#include "VM.h"

const std::string threadStatusToStr(unsigned char c) {
    switch(c) {
        case 0:
            return "THREAD_READY";
        case 1:
            return "THREAD_BLOCKED";
        case 2:
            return "THREAD_FINISHED";
        case 3:
            return "THREAD_WAITING";
        default:
            return "UNKNOWN_STATE (" + std::to_string(c) + ")";
    }
}


Thread::Thread(std::string name, Function *currectFunction) {
    this->name = name;
    this->currentFunction = currectFunction;
    this->status = THREAD_READY;
    this->priority = 5;
}

Thread::~Thread() {
    while(this->currentFunction != nullptr) {
        auto previousFunction = currentFunction->returnFunction;
        currentFunction->returnFunction = nullptr;
        delete currentFunction;
        currentFunction = previousFunction;
    }
    for(auto it = this->joiningThreads.begin(); it != joiningThreads.end(); it++)
        (*it)->unblock();
}

void Thread::run() {
    if(this->currentFunction != nullptr)
        this->currentFunction->run();

    if(this->currentFunction == nullptr)
        this->status = THREAD_FINISHED;
    else {
        if(this->currentFunction->blocked)
            this->status = THREAD_BLOCKED;
        else if(this->currentFunction->waiting)
            this->status = THREAD_WAITING;
        else
            this->status = THREAD_READY;
        this->currentFunction->waiting = false;
        this->currentFunction->anotherFunctionCalled = false;
    }
}

void Thread::joining(Thread *joiningThread) {
    this->joiningThreads.push_back(joiningThread);
}

void Thread::unblock() {
    this->status = THREAD_READY;
    this->currentFunction->blocked = false;
}

void Thread::receive(int value) {
    this->recvTable.push_back(value);
    #if DEBUG == 1
    VM::getVM().refresh();
    #endif
}

#if DEBUG == 1
void Thread::refresh(WINDOW *window, bool isCurrent) {
    wclear(window);
    box(window, 0 , 0);

    auto threadWinHeight = VM::getVM().threadWinHeight;
    threadWinHeight -= 4;
    int maxCodeLinesDisplay = (int)(threadWinHeight * 0.5);
    int maxArgsDisplay = (int)(threadWinHeight * 0.25);
    int maxRecvDisplay = (int)(threadWinHeight * 0.25);

    // add name and code
    if(isCurrent)
        wattron(window, COLOR_PAIR(3));
    else
        wattron(window, COLOR_PAIR(5));

    auto lines = this->currentFunction->toStr();
    unsigned int yPos = 1;
    for(int i = 0; i < maxCodeLinesDisplay && yPos - 1 < lines.size(); yPos++, i++)
        mvwaddstr(window, yPos, 1, lines.at((unsigned long)yPos-1).c_str());

    // add code
    if(isCurrent)
        mvwaddstr(window, 1, (int)lines.at(0).size() + 2,
              (std::string("[" + this->name) + " | " + std::to_string(this->priority) + "] *").c_str());
    else
        mvwaddstr(window, 1, (int)lines.at(0).size() + 2,
                  (std::string("[" + this->name) + " | " + std::to_string(this->priority) + "]").c_str());

    // add vpc
    int headerSize = 3;
    mvwaddstr(window, int(headerSize + this->currentFunction->vpc),
              int(lines.at((unsigned long)headerSize + this->currentFunction->vpc - 1).size() + 3), "<");

    // add args to code
    yPos += 2;
    mvwaddstr(window, yPos, 1, "ARGS:");
    yPos += 1;

    auto dttArgsDispIt = this->currentFunction->dttArgsIt;
    for(int i = 0; i < maxArgsDisplay && dttArgsDispIt != this->currentFunction->dttArgs->end(); i++, yPos++, dttArgsDispIt++) {
        std::string arg = argTypeToStr((*dttArgsDispIt).type) + "\t- ";
        if((*dttArgsDispIt).type != CONST)
            arg += (*dttArgsDispIt).valStr + " ";
        if((*dttArgsDispIt).type == VAR || (*dttArgsDispIt).type == CONST) {
            int argVal = (*dttArgsDispIt).valInt;
            if ((*dttArgsDispIt).type == VAR)
                argVal = this->currentFunction->varTable[(*dttArgsDispIt).valStr];
            arg += std::to_string(argVal);
        }
        mvwaddstr(window, yPos, 1, arg.c_str());
    }

    // add recv table
    yPos += 2;
    mvwaddstr(window, yPos, 1, "RECV: [");
    yPos += 1;

    auto recvTableDispIt = this->recvTable.begin();
    for(int i = 0; i < maxRecvDisplay && recvTableDispIt != this->recvTable.end(); i++, yPos++, recvTableDispIt++) {
        std::string arg = "\t" + std::to_string(*recvTableDispIt);
        mvwaddstr(window, yPos, 1, arg.c_str());
    }
    mvwaddstr(window, yPos, 1, "]");

    wrefresh(window);
}
#endif


ThreadManager::ThreadManager() {
    this->scheduler = nullptr;
    this->currentThread = nullptr;
}

ThreadManager::~ThreadManager() {
    this->clearAll();
}

void ThreadManager::clearAll() {
    for(auto it = this->threads.begin(); it != this->threads.end(); it++) {
        delete *it;
    }
    this->threads.clear();
    delete this->scheduler;
    this->scheduler = nullptr;
}

Thread* ThreadManager::addThread(std::string threadName, Function* threadFunction) {
    auto threadExists = std::find_if(this->threads.begin(), this->threads.end(),
                               [&threadName](const Thread* threadTmp) {return threadTmp->name == threadName;});
    if(threadExists != this->threads.end())
        throw VMRuntimeException("Thread with name " + threadName + " already exists");

    auto thread = new Thread(threadName, threadFunction);
    this->threads.push_back(thread);
    return thread;
}

void ThreadManager::removeThread(std::string threadName) {
    auto thread = std::find_if(this->threads.begin(), this->threads.end(),
                           [&threadName](const Thread* threadTmp) {return threadTmp->name == threadName;});
    if(thread != this->threads.end()) {
        if(this->currentThread == *thread)
            throw VMRuntimeException("Can't delete currently running thread");
        delete *thread;
        this->threads.erase(thread);
    }
}

void ThreadManager::schedule() {
    if(this->threads.size() == 0)
        throw VMRuntimeException("No threads in ThreadManager");

    while(this->threads.size() > 0) {
        this->currentThread = this->scheduler->schedule(this->currentThread, this->threads);
        if(this->currentThread != nullptr)
            this->currentThread->run();
    }
}

void ThreadManager::changeScheduler(ThreadScheduler *scheduler) {
    this->scheduler = scheduler;
    this->scheduler->initialize();
}

Function *ThreadManager::getCurrentFunction() {
    return this->currentThread->currentFunction;
}

Thread* ThreadManager::getCurrentThread() {
    return this->currentThread;
}

Thread* ThreadManager::getThread(std::string threadName) {
    auto thread = std::find_if(this->threads.begin(), this->threads.end(),
                               [&threadName](const Thread* threadTmp) {return threadTmp->name == threadName;});
    if(thread == this->threads.end())
        return nullptr;
    return *thread;
}

void ThreadManager::checkAllThreadsWaiting() {
    for(auto it = this->threads.begin(); it != this->threads.end(); it++)
        if((*it)->status == THREAD_READY)
            return;

    std::string threadsStatuses = "";
    for(auto it = this->threads.begin(); it != this->threads.end(); it++)
        threadsStatuses += "\n" + (*it)->name + " - " + threadStatusToStr((*it)->status);
    throw VMRuntimeException("All threads blocked, statuses: " + threadsStatuses);
}

#if DEBUG == 1
void ThreadManager::refreshThreads(std::vector<WINDOW*> windows, unsigned int startThread) {
    unsigned int i = 0;
    bool isCurrent;
    for(; i < windows.size() && i + startThread < this->threads.size(); i++){
        isCurrent = false;
        if(this->currentThread == this->threads.at((unsigned long)i + startThread))
            isCurrent = true;
        this->threads.at((unsigned long)i + startThread)->refresh(windows.at((unsigned long)i), isCurrent);
    }
    for(; i < windows.size(); i++) {
        wclear(windows.at((unsigned long)i));
        wrefresh(windows.at((unsigned long)i));
    }
}

#endif



Thread *ThreadScheduler::schedule(Thread *currentThread, std::vector<Thread *> &threads) {
    if(threads.size() == 0)
        throw VMRuntimeException("Scheduling without any thread");

    if(currentThread == nullptr)
        return threads.at(0);

    if(currentThread->status == THREAD_FINISHED) {
        if(threads.size() == 1) {
            threads.clear();
        } else {
            auto currentThreadIt = std::find(threads.begin(), threads.end(), currentThread);
            delete *currentThreadIt;
            threads.erase(currentThreadIt);
        }
    }
    return nullptr;
}

std::string ThreadScheduler::getName() {
    return this->name;
}


FIFOScheduler::FIFOScheduler() : ThreadScheduler("FIFO") {}

void FIFOScheduler::initialize(){
    if(!this->initialized) {
        VM::getVM().setSchedulingFrequency(0);
        this->initialized = true;
    }
}

Thread* FIFOScheduler::schedule(Thread* currentThread, std::vector<Thread*>& threads) {
    auto newThread = ThreadScheduler::schedule(currentThread, threads);
    if(newThread != nullptr)
        return newThread;

    if(threads.empty())
        return nullptr;

    for(auto it = threads.begin(); it != threads.end(); it++) {
        if((*it)->status == THREAD_READY)
            return *it;
    }
    throw VMRuntimeException("All threads blocked");
}


RoundRobinScheduler::RoundRobinScheduler() : ThreadScheduler("RoundRobin") {}

void RoundRobinScheduler::initialize() {
    if(!this->initialized) {
        VM::getVM().setSchedulingFrequency(5);
        this->initialized = true;
    }
}

Thread* RoundRobinScheduler::schedule(Thread* currentThread, std::vector<Thread*>& threads) {
    auto newThread = ThreadScheduler::schedule(currentThread, threads);
    if(newThread != nullptr)
        return newThread;

    if(threads.empty())
        return nullptr;

    if(currentThread == nullptr || std::find(threads.begin(), threads.end(), currentThread) == threads.end())
        currentThread = threads.at(0);

    auto currentThreadIt = std::find(threads.begin(), threads.end(), currentThread);
    if(currentThreadIt == threads.end())
        currentThreadIt = threads.begin();

    for(auto it = currentThreadIt + 1; it != threads.end(); it++) {
        if((*it)->status != THREAD_BLOCKED) {
            return *it;
        }
    }
    for(auto it = threads.begin(); it != currentThreadIt; it++) {
        if((*it)->status == THREAD_READY) {
            return *it;
        }
    }
    if((*currentThreadIt)->status != THREAD_BLOCKED) {
        return *currentThreadIt;
    }
    throw VMRuntimeException("All threads blocked");
}


PriorityScheduler::PriorityScheduler() : ThreadScheduler("Priority") {}

void PriorityScheduler::initialize() {
    if(!this->initialized) {
        VM::getVM().setSchedulingFrequency(5);
        this->initialized = true;
    }
}

Thread* PriorityScheduler::schedule(Thread* currentThread, std::vector<Thread*>& threads) {
    auto newThread = ThreadScheduler::schedule(currentThread, threads);
    if(newThread != nullptr)
        return newThread;

    if(threads.empty())
        return nullptr;

    if(currentThread == nullptr || std::find(threads.begin(), threads.end(), currentThread) == threads.end())
        currentThread = threads.at(0);

    std::vector<Thread*> equalPriorityThreads;
    for(auto it = threads.begin(); it != threads.end(); it++) {
        if((*it)->priority > currentThread->priority)
            return *it;
        else if((*it)->priority == currentThread->priority)
            equalPriorityThreads.push_back(*it);
    }

    std::sort( equalPriorityThreads.begin( ), equalPriorityThreads.end( ), [ ]( const Thread* lhs, const Thread* rhs ) {
        return lhs->name < rhs->name;
    });

    auto currentThreadIt = std::find_if(equalPriorityThreads.begin(), equalPriorityThreads.end(),
                           [&currentThread](const Thread* obj) {return obj == currentThread;});
    currentThreadIt++;
    if(currentThreadIt == equalPriorityThreads.end())
        return equalPriorityThreads.at(0);
    else
        return *currentThreadIt;
}
