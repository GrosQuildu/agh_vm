//
// Created by gros on 13.12.17.
//

#include <iostream>
#include "VM.h"

using namespace std;


int main() {
    cout<<"Start\n";

    auto codeDir = "/home/gros/studia/eaiib_3/wzorce/virtual_machine/example_program/";
    auto blocksDir = "/home/gros/studia/eaiib_3/wzorce/virtual_machine/blocks/";
    auto defaultScheduler = "RoundRobin";
    auto rebuild = true;

    VM::getVM().initialize(codeDir, blocksDir, defaultScheduler, rebuild);
    VM::getVM().start();
}