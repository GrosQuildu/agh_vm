//
// Created by gros on 13.12.17.
//

#include "Thread.h"


Thread::Thread(std::string name, Function *currect_function) {
    this->name = name;
    this->currect_function = currect_function;
    this->status = THREAD_READY;
}

void Thread::run() {
    this->currect_function->run();
}


ThreadManager::ThreadManager() {
    this->threadSchedulers.push_back(new FIFOScheduler());
    this->threadSchedulers.emplace_back(new RoundRobinScheduler());
    this->changeScheduler(this->threadSchedulers.at(0));
}

void ThreadManager::add(Thread& thread) {
    this->threads.push_back(thread);
}

void ThreadManager::remove(int position) {
    if(position < this->threads.size() && position >= 0) {
        this->threads.erase(this->threads.begin()+position);
        if(position <= this->current_thread)
            this->current_thread--;
    }
}

void ThreadManager::schedule() {
    if(this->scheduler == nullptr)
        throw VMRuntimeException("Scheduler not set");
    if(this->threads.size() == 0)
        throw VMRuntimeException("No threads in ThreadManager");

    this->current_thread = this->scheduler->schedule(this->current_thread, this->threads);
    this->threads.at(this->current_thread).run();
}

void ThreadManager::changeScheduler(ThreadScheduler *scheduler) {
    this->scheduler = scheduler;
}

Function *ThreadManager::getCurrentFunction() {
    return this->getCurrentThread()->currect_function;
}

Thread *ThreadManager::getCurrentThread() {
    return &this->threads.at(this->current_thread);
}


unsigned long FIFOScheduler::schedule(unsigned long current_thread, std::vector<Thread>& threads) {
    for(auto it = threads.begin(); it != threads.end(); it++) {
        if((*it).status != THREAD_BLOCKED)
            return (unsigned long)std::distance(threads.begin(), it);
    }
    throw VMRuntimeException("All threads blocked");
}

unsigned long RoundRobinScheduler::schedule(unsigned long current_thread, std::vector<Thread>& threads) {
    for(auto it = threads.begin() + current_thread + 1; it != threads.begin() + current_thread + 1; it++) {
        if(it == threads.end())
            it = threads.begin();

        if((*it).status != THREAD_BLOCKED) {
            return (unsigned long) std::distance(threads.begin(), it);
        }
    }
    throw VMRuntimeException("All threads blocked");
}
