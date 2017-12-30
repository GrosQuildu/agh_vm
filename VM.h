//
// Created by gros on 13.12.17.
//

#ifndef VM_VM_H
#define VM_VM_H

#include <set>
#include "Function.h"

using std::set;


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

    void start();
    Function* getCurrentFunction();
    Function *currentFunction_;

private:
    static bool isInitialized;
    static FunctionFactory functionFactory;
};


#endif //VM_VM_H
