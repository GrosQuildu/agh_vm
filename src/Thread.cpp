//
// Created by gros on 13.12.17.
//

#include "Thread.h"
#include "VM.h"


Thread::Thread(std::string name, Function *currect_function) {
    this->name = name;
    this->currect_function = currect_function;
    this->status = THREAD_READY;
    this->reshedule = false;
}

Thread::~Thread() {
    while(this->currect_function != nullptr) {
        auto previousFunction = currect_function->returnFunction;
        currect_function->returnFunction = nullptr;
        delete currect_function;
        currect_function = previousFunction;
    }
}

void Thread::run() {
    while(this->currect_function != nullptr && !this->reshedule)
        this->currect_function->run();

    if(this->currect_function == nullptr)
        this->status = THREAD_FINISHED;

    if(this->reshedule) {
        this->reshedule = false;
        this->currect_function->anotherFunctionCalled = false;
    }
}

#if DEBUG == 1
void Thread::refresh(WINDOW *window) {
    wclear(window);
    box(window, 0 , 0);

    int maxCodeLinesDisplay = 30;
    int maxArgsDisplay = 10;

    // add name and code
    auto lines = this->currect_function->toStr();
    unsigned int yPos = 1;
    for(int i = 0; i < maxCodeLinesDisplay && yPos - 1 < lines.size(); yPos++, i++)
        mvwaddstr(window, yPos, 1, lines.at((unsigned long)yPos-1).c_str());

    // add vpc
    int headerSize = 3;
    auto vpcOffset = std::distance(this->currect_function->dtt->begin(), this->currect_function->vpc) - 1;
    if(vpcOffset < 0)
        vpcOffset = 0;
    mvwaddstr(window, int(headerSize + vpcOffset), int(lines.at((unsigned long)headerSize + vpcOffset - 1).size() + 3), "<");

    // add args to code
    yPos += 2;
    mvwaddstr(window, yPos, 1, "ARGS:");
    yPos += 1;

    auto it = this->currect_function->dtt_args->begin();
    for(int i = 0; i < maxArgsDisplay && it != this->currect_function->dtt_args->end(); i++, yPos++, it++) {
        std::string arg = const2str((*it).type) + "\t- ";
        if((*it).type != CONST)
            arg += (*it).valStr + " ";
        if((*it).type == VAR || (*it).type == CONST) {
            int argVal = (*it).valInt;
            if ((*it).type == VAR)
                argVal = this->currect_function->var_table[(*it).valStr];
            arg += std::to_string(argVal);
        }
        mvwaddstr(window, yPos, 1, arg.c_str());
    }

    wrefresh(window);
}
#endif


ThreadManager::ThreadManager() {
    this->scheduler = nullptr;
    this->current_thread = nullptr;
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
    auto thread = new Thread(threadName, threadFunction);
    this->threads.push_back(thread);
    return thread;
}

void ThreadManager::removeThread(std::string threadName) {
    auto thread = std::find_if(this->threads.begin(), this->threads.end(),
                           [&threadName](const Thread* threadTmp) {return threadTmp->name == threadName;});
    if(thread != this->threads.end()) {
        if(this->current_thread == *thread)
            throw VMRuntimeException("Can't delete currently running thread");
        delete *thread;
        this->threads.erase(thread);
    }
}

void ThreadManager::schedule() {
    if(this->threads.size() == 0)
        throw VMRuntimeException("No threads in ThreadManager");

    while(this->threads.size() > 0) {
        this->current_thread = this->scheduler->schedule(this->current_thread, this->threads);
        if(this->current_thread != nullptr)
            this->current_thread->run();
    }
}

void ThreadManager::changeScheduler(ThreadScheduler *scheduler) {
    this->scheduler = scheduler;
    this->scheduler->initialize();
}

Function *ThreadManager::getCurrentFunction() {
    return this->current_thread->currect_function;
}

Thread* ThreadManager::getCurrentThread() {
    return this->current_thread;
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



Thread *ThreadScheduler::schedule(Thread *current_thread, std::vector<Thread *> &threads) {
    if(threads.size() == 0)
        throw VMRuntimeException("Scheduling without any thread");

    if(current_thread == nullptr)
        return threads.at(0);

    if(current_thread->status == THREAD_FINISHED) {
        if(threads.size() == 1) {
//            delete current_thread;
            threads.clear();
        } else {
            auto currentThreadIt = std::find(threads.begin(), threads.end(), current_thread);
            delete *currentThreadIt;
            threads.erase(currentThreadIt);
            return *currentThreadIt;
        }
    }

    if(threads.size() == 1)
        return threads.at(0);

    return nullptr;
}

std::string ThreadScheduler::getName() {
    return this->name;
}


FIFOScheduler::FIFOScheduler() : ThreadScheduler("FIFO") {}

void FIFOScheduler::initialize(){
    if(!this->initialized) {
        VM::setSchedulingFrequency(0);
        this->initialized = true;
    }
}

Thread* FIFOScheduler::schedule(Thread* current_thread, std::vector<Thread*>& threads) {
    auto newThreadIt = ThreadScheduler::schedule(current_thread, threads);
    if(newThreadIt != nullptr)
        return newThreadIt;

    if(threads.size() == 0)
        return nullptr;

    for(auto it = threads.begin(); it != threads.end(); it++) {
        if((*it)->status != THREAD_BLOCKED)
            return *it;
    }
    throw VMRuntimeException("All threads blocked");
}

RoundRobinScheduler::RoundRobinScheduler() : ThreadScheduler("RoundRobin") {}

void RoundRobinScheduler::initialize() {
    if(!this->initialized) {
        VM::setSchedulingFrequency(5);
        this->initialized = true;
    }
}

Thread* RoundRobinScheduler::schedule(Thread* current_thread, std::vector<Thread*>& threads) {
    auto newThreadIt = ThreadScheduler::schedule(current_thread, threads);
    if(newThreadIt != nullptr)
        return newThreadIt;

    if(threads.size() == 0)
        return nullptr;

    auto currentThreadIt = std::find(threads.begin(), threads.end(), current_thread);
    for(auto it = currentThreadIt + 1; it != currentThreadIt; it++) {
        if(it == threads.end())
            it = threads.begin();

        if((*it)->status != THREAD_BLOCKED) {
            return *it;
        }
    }
    throw VMRuntimeException("All threads blocked");
}

