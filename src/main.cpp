//
// Created by gros on 13.12.17.
//

#include <iostream>
#include <cstring>
#include "VM.h"

using namespace std;


int main(int argc, char *argv[]) {

    if(argc >= 2) {
        if (strncmp(argv[1], "-h", 2) == 0 || strncmp(argv[1], "--help", 5) == 0) {
            if (argc > 0) {
                cout << "Usage: " << argv[0] << " [--help | -h] codeDir blocksDir scheduler rebuild\n";
            } else {
                cout << "Usage: ./filename [--help | -h] codeDir blocksDir scheduler rebuild\n";
            }
            cout << "\n    codeDir - path to directory with bytecode for virtual machine\n";
            cout << "    blocksDir - path to directory where some tmp files (blocks to compile) wil be stored\n";
            cout << "    scheduler - [Priority, FIFO, RoundRobin]\n";
            cout << "    rebuild - [true | false] if false, try to use existing blocks from blocksDir\n";
            exit(EXIT_SUCCESS);
        }
    }

    if(argc < 5) {
        if(argc > 0) {
            cerr << "Usage: " << argv[0] << " [--help | -h] codeDir blocksDir scheduler rebuild\n";
        }
        else {
            cerr << "Usage: ./filename [--help | -h] codeDir blocksDir scheduler rebuild\n";
        }
        exit(EXIT_FAILURE);
    }

    auto codeDir =  string(argv[1]) + "/";
    auto blocksDir = string(argv[2]) + "/";
    auto defaultScheduler = argv[3];
    auto rebuild = bool(argv[4]);

    cout<<"Start\n";
    VM::getVM().initialize(codeDir, blocksDir, defaultScheduler, rebuild);
    VM::getVM().start();
    VM::getVM().stop();
}