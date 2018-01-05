//
// Created by gros on 13.12.17.
//

#include <iostream>
#include "VM.h"

using namespace std;


int main() {
    cout<<"Start\n";
    auto codeDir = "/home/gros/studia/eaiib_3/wzorce/virtual_machine/example_program/";
    auto defaultScheduler = "FIFO";
    VM::getVM().initialize(codeDir, defaultScheduler);
    VM::getVM().start();
}