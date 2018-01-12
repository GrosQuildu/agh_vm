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
    void initialize(const std::string, const std::string);

    /**
     * Get virtual machine singleton instance
     * @return VM&
     */
    static VM& getVM();  // singleton

    static void start();
    static void stop();

    static Function* getCurrentFunction();
    static int getCurrentFunctionNextArgInt();
    static std::string getCurrentFunctionNextArgStr();
    static void setVarTable(std::string, int);
    static Thread* getCurrentThread();
    static Function* getNewFunction(std::string);
    static Thread* getNewThread(std::string, std::string);
    static void stopThread(std::string);

    static bool changeScheduler(std::string);
    static void setMaxBlockSize(unsigned long);

    static void print(std::string);

private:
    static bool isInitialized;
    static FunctionFactory functionFactory;
    static ThreadManager threadManager;
    static std::vector<ThreadScheduler*> threadSchedulers;

    static int ThreadWinWidth;
    static int ThreadWinHeight;
    static int ThreadWinMargin;

    #if DEBUG == 1
    static std::vector<WINDOW*> windows;
    static WINDOW* terminal;
    static void refresh();
    #endif
};


#endif //VM_VM_H
