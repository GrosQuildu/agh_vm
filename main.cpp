//
// Created by gros on 13.12.17.
//

#include <iostream>
#include "VM.h"

using namespace std;


int main() {
    cout<<"Start\n";
    string codeDir = "/home/gros/studia/eaiib_3/wzorce/virtual_machine/example_program/";
    VM::initialize(codeDir);
    VM::getVM().start();
}