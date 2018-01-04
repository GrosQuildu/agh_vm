//
// Created by gros on 13.12.17.
//

#ifndef VM_THREAD_H
#define VM_THREAD_H


#include "Function.h"
#include "Exceptions.h"
#include <ncurses.h>

const char THREAD_READY = 0;
const char THREAD_BLOCKED = 1;
const char THREAD_FINISHED = 2;

class Thread {
public:
    Thread(std::string, Function*);
    ~Thread();
    void run();

    std::string name;
    Function* currect_function;
    int status;
    std::vector<int> recv_table;
    bool reshedule;

    void refresh(WINDOW*);
};


class ThreadScheduler {
public:
    /**
     * @param int current_thread
     * @param vector<Thread>& threads
     * @return unsigned long - next thread to run
     */
    virtual void initialize() = 0;
    virtual Thread* schedule(Thread* current_thread, std::vector<Thread*>& threads);

protected:
    std::string name;
    bool initialized;

    ThreadScheduler(std::string name) : name{name}, initialized{false} {};
};


class FIFOScheduler : public ThreadScheduler {
public:
    FIFOScheduler();
    void initialize();
    Thread* schedule(Thread* current_thread, std::vector<Thread*>& threads);
};

class RoundRobinScheduler : public ThreadScheduler {
public:
    RoundRobinScheduler();
    void initialize();
    Thread* schedule(Thread* current_thread, std::vector<Thread*>& threads);
};



class ThreadManager {
public:
    ThreadManager();
    ~ThreadManager();

    Thread* addThread(std::string, Function*);
    void removeThread(std::string);
    void clearAll();

    void schedule();
    void changeScheduler(ThreadScheduler*);

    Function* getCurrentFunction();
    Thread* getCurrentThread();

    void refreshThreads(std::vector<WINDOW*>, int);

private:
    std::vector<Thread*> threads;
    Thread* current_thread;
    ThreadScheduler* scheduler;
};

#endif //VM_THREAD_H
