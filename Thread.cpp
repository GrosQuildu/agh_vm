//
// Created by gros on 13.12.17.
//

#include "Thread.h"

Thread::Thread(std::string name, Function *currect_function) {
    this->name = name;
    this->currect_function = currect_function;
    this->status = THREAD_READY;
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
    while(this->currect_function != nullptr)
        this->currect_function->run();
}


ThreadManager::ThreadManager() {
    this->threadSchedulers.push_back(new FIFOScheduler());
    this->threadSchedulers.emplace_back(new RoundRobinScheduler());
    this->changeScheduler(this->threadSchedulers.at(0));
}

ThreadManager::~ThreadManager() {
    this->clearAll();
}

void ThreadManager::clearAll() {
    for(auto it = this->threads.begin(); it != this->threads.end(); it++) {
        delete *it;
    }
    for(auto it = this->threadSchedulers.begin(); it != this->threadSchedulers.end(); it++) {
        delete *it;
    }
    this->threads.clear();
    this->threadSchedulers.clear();
}

Thread* ThreadManager::addThread(std::string threadName, Function* threadFunction) {
    auto thread = new Thread(threadName, threadFunction);
    this->threads.push_back(thread);
    return thread;
}

void ThreadManager::removeThread(std::string threadName) {
    auto thread= std::find_if(this->threads.begin(), this->threads.end(),
                           [&threadName](const Thread* thread) {return thread->name == threadName;});
    if(thread != this->threads.end()) {
        delete *thread;
        this->threads.erase(thread);
        if(this->current_thread > 0) {
            auto index = std::distance(this->threads.begin(), thread);
            if (index <= this->current_thread)
                this->current_thread--;
        }
    }
    if(this->threadSchedulers.empty())
        throw ThreadManagerException("All threads removed");
}

void ThreadManager::schedule() {
    if(this->scheduler == nullptr)
        throw VMRuntimeException("Scheduler not set");
    if(this->threads.size() == 0)
        throw VMRuntimeException("No threads in ThreadManager");

    this->current_thread = this->scheduler->schedule(this->current_thread, this->threads);
    this->threads.at(this->current_thread)->run();
}

void ThreadManager::changeScheduler(ThreadScheduler *scheduler) {
    this->scheduler = scheduler;
}

Function *ThreadManager::getCurrentFunction() {
    return this->getCurrentThread()->currect_function;
}

Thread *ThreadManager::getCurrentThread() {
    return this->threads.at(this->current_thread);
}



unsigned long FIFOScheduler::schedule(unsigned long current_thread, std::vector<Thread*>& threads) {
    for(auto it = threads.begin(); it != threads.end(); it++) {
        if((*it)->status != THREAD_BLOCKED)
            return (unsigned long)std::distance(threads.begin(), it);
    }
    throw VMRuntimeException("All threads blocked");
}

unsigned long RoundRobinScheduler::schedule(unsigned long current_thread, std::vector<Thread*>& threads) {
    for(auto it = threads.begin() + current_thread + 1; it != threads.begin() + current_thread + 1; it++) {
        if(it == threads.end())
            it = threads.begin();

        if((*it)->status != THREAD_BLOCKED) {
            return (unsigned long) std::distance(threads.begin(), it);
        }
    }
    throw VMRuntimeException("All threads blocked");
}
