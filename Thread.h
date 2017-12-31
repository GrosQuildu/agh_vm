//
// Created by gros on 13.12.17.
//

#ifndef VM_THREAD_H
#define VM_THREAD_H


#include "Function.h"
#include "Exceptions.h"


const char THREAD_READY = 0;
const char THREAD_BLOCKED = 1;

class Thread {
public:
    Thread(std::string, Function*);
    void run();

    std::string name;
    Function* currect_function;
    int status;
    std::vector<int> recv_table;
};


class ThreadScheduler {
public:
    /**
     * @param int current_thread
     * @param vector<Thread>& threads
     * @return unsigned long - next thread to run
     */
    virtual unsigned long schedule(unsigned long, std::vector<Thread>&) = 0;
    std::string name;

protected:
    ThreadScheduler(std::string name) : name{name} {};
};


class FIFOScheduler : public ThreadScheduler {
public:
    FIFOScheduler() : ThreadScheduler("FIFO") {};
    unsigned long schedule(unsigned long, std::vector<Thread>&);
};

class RoundRobinScheduler : public ThreadScheduler {
public:
    RoundRobinScheduler() : ThreadScheduler("RoundRobin") {};
    unsigned long schedule(unsigned long, std::vector<Thread>&);
};



class ThreadManager {
public:
    ThreadManager();
    void add(Thread&);
    void remove(int);
    void schedule();
    void changeScheduler(ThreadScheduler*);

    Function* getCurrentFunction();
    Thread* getCurrentThread();

private:
    std::vector<Thread> threads;
    unsigned long current_thread;
    ThreadScheduler *scheduler = nullptr;
    std::vector<ThreadScheduler*> threadSchedulers;
};

#endif //VM_THREAD_H
