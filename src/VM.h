//
// Created by gros on 13.12.17.
//

#ifndef VM_VM_H
#define VM_VM_H

#include "Thread.h"

class VM {
public:
    /** Initialize virtual machine singleton
     * @param string codePathDir
     */
    void initialize(const std::string, const std::string, const std::string, bool);

    /**
     * Get virtual machine singleton instance
     * @return VM&
     */
    static VM& getVM();  // singleton

    void start();
    void stop();
    void checkAllThreadsWaiting();

    Function* getCurrentFunction(bool = true);
    Function* getNewFunction(std::string);

    Thread* getNewThread(std::string, std::string);
    Thread* getCurrentThread();
    void stopThread(std::string);

    Thread* getThread(std::string);

    bool changeScheduler(std::string);
    void setSchedulingFrequency(unsigned long);

    void print(std::string);

    bool rebuild;
    std::string blocksDir;

private:
    bool isInitialized;
    FunctionFactory *functionFactory;
    ThreadManager *threadManager;
    std::vector<ThreadScheduler*> *threadSchedulers;

    int ThreadWinWidth;
    int ThreadWinHeight;
    int ThreadWinMargin;

    #if DEBUG == 1
    std::vector<WINDOW*> windows;
    WINDOW* terminal;
    void refresh();
    #endif
};


#endif //VM_VM_H
