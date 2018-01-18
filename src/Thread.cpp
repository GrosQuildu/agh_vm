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
}

#if DEBUG == 1
void Thread::refresh(WINDOW *window) {
    wclear(window);
    box(window, 0 , 0);

    int maxCodeLinesDisplay = 30;
    int maxArgsDisplay = 10;

    // add name and code
    auto lines = this->currentFunction->toStr();
    unsigned int yPos = 1;
    for(int i = 0; i < maxCodeLinesDisplay && yPos - 1 < lines.size(); yPos++, i++)
        mvwaddstr(window, yPos, 1, lines.at((unsigned long)yPos-1).c_str());

    // add vpc
    int headerSize = 3;
    auto vpcOffset = std::distance(this->currentFunction->dtt->begin(), this->currentFunction->vpc);
    mvwaddstr(window, int(headerSize + vpcOffset), int(lines.at((unsigned long)headerSize + vpcOffset - 1).size() + 3), "<");

    // add args to code
    yPos += 2;
    mvwaddstr(window, yPos, 1, "ARGS:");
    yPos += 1;

    auto it = this->currentFunction->dttArgs->begin();
    for(int i = 0; i < maxArgsDisplay && it != this->currentFunction->dttArgs->end(); i++, yPos++, it++) {
        std::string arg = const2str((*it).type) + "\t- ";
        if((*it).type != CONST)
            arg += (*it).valStr + " ";
        if((*it).type == VAR || (*it).type == CONST) {
            int argVal = (*it).valInt;
            if ((*it).type == VAR)
                argVal = this->currentFunction->varTable[(*it).valStr];
            arg += std::to_string(argVal);
        }
        mvwaddstr(window, yPos, 1, arg.c_str());
    }

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
    for(; i < windows.size() && i + startThread < this->threads.size(); i++){
        this->threads.at((unsigned long)i + startThread)->refresh(windows.at((unsigned long)i));
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

