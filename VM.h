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
    static void initialize(const std::string&);

    /**
     * Get virtual machine singleton instance
     * @return VM&
     */
    static VM& getVM();  // singleton

    static void start();
    static void stop();

    static Function* getCurrentFunction();
    static Thread* getCurrentThread();
    static Function* getNewFunction(std::string);
    static Thread* getNewThread(std::string, std::string);
    static void stopThread(std::string);

private:
    static bool isInitialized;
    static FunctionFactory functionFactory;
    static ThreadManager threadManager;
};


#endif //VM_VM_H
