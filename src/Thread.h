//
// Created by gros on 13.12.17.
//

#ifndef VM_THREAD_H
#define VM_THREAD_H

#if DEBUG == 1
#include <ncurses.h>
#endif

#include "Function.h"
#include "Exceptions.h"


const char THREAD_READY = 0;
const char THREAD_BLOCKED = 1;
const char THREAD_FINISHED = 2;
const char THREAD_WAITING = 3;

const std::string threadStatusToStr(char);

class Thread {
public:
    Thread(std::string, Function*);
    ~Thread();
    void run();

    void joining(Thread*);
    void unblock();
    void receive(int);

    std::string name;
    Function* currect_function;
    unsigned char status;
    std::vector<int> recv_table;
    std::vector<Thread*> joiningThreads;

    #if DEBUG == 1
    void refresh(WINDOW*);
    #endif
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
    virtual std::string getName();
    virtual ~ThreadScheduler() {};

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
    void checkAllThreadsWaiting();

    Function* getCurrentFunction();
    Thread* getCurrentThread();
    Thread* getThread(std::string);

    #if DEBUG == 1
    void refreshThreads(std::vector<WINDOW*>, unsigned int);
    #endif

private:
    std::vector<Thread*> threads;
    Thread* current_thread;
    ThreadScheduler* scheduler;
};

#endif //VM_THREAD_H
