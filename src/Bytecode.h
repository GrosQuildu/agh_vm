//
// Created by gros on 18.01.18.
//

#ifndef VM_BYTECODE_H
#define VM_BYTECODE_H

#include <string>

std::string vm_prolog(std::string);
std::string vm_epilog();
std::string vm_schedule();
std::string vm_assign();
std::string vm_print();
std::string vm_call();
std::string vm_return();
std::string vm_send();
std::string vm_recv();
std::string vm_start();
std::string vm_join();
std::string vm_stop();
std::string vm_add();
std::string vm_sub();
std::string vm_div();
std::string vm_mul();

#endif //VM_BYTECODE_H
