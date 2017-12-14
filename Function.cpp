//
// Created by gros on 13.12.17.
//

#include "Function.h"
#include "VM.h"

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>

void vm_add() {
    auto cf = VM::getVM("").getCurrentFunction();
    auto arg0 = cf->dtt_args[cf->arg_c++];
    auto arg1 = cf->dtt_args[cf->arg_c++];
    auto arg2 = cf->dtt_args[cf->arg_c++];

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = cf->var_table[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = cf->var_table[arg2.valStr];

    cf->var_table[arg0.valStr] = val1 + val2;
}

void vm_print() {
    auto cf = VM::getVM("").getCurrentFunction();
    auto arg0 = cf->dtt_args[cf->arg_c++];

    int val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = cf->var_table[arg0.valStr];

    std::cout << val0 << "\n";
}
void vm_return() {
    return;
}




static std::map<dtt_type, std::string> bytecodeMapping = {
        {vm_add, "VM_DD"},
        {vm_print, "VM_PRINT"},
        {vm_return, "VM_RETURN"},
};





std::map<std::string, FunctionPrototype*> FunctionFactory::functionsPrototypes;

/**
 * Add function to FunctionFactory from file
 * @param codePath
 */
void FunctionFactory::addFunction(std::string codePath) {
    auto functionPrototype = parseCode(codePath);
    FunctionFactory::functionsPrototypes[functionPrototype->name] = functionPrototype;
}

/**
 * Parse bytecode to FunctionPrototype
 * @param string - function's code to parse
 * @return
     * string - function name
     * dtt_type[] - dtt table (array of pointers to functions that gets void and returns void)
     * dtt_arg[] - table of dtt_arg structs, arguments for functions stored in dtt table
     * int[] - function's arguments table
     * map<string, int> - function's variables table
 */
FunctionPrototype* FunctionFactory::parseCode(std::string) {

    std::string name = "MAIN";

    dtt_type* dtt = new dtt_type[3];
    int dtt_size = 3;

    dtt[0] = vm_add;
    dtt[1] = vm_print;
    dtt[2] = vm_return;

    dtt_arg *dtt_args = new dtt_arg[5];
    int dtt_args_size = 5;

    dtt_args[0].type = VAR;
    dtt_args[0].valStr = "ZMIENNA1";

    dtt_args[1].type = CONST;
    dtt_args[1].valInt = 1;

    dtt_args[2].type = CONST;
    dtt_args[2].valInt = -4;

    dtt_args[3].type = VAR;
    dtt_args[3].valStr = "ZMIENNA1";

    dtt_args[4].type = VAR;
    dtt_args[4].valStr = "ZMIENNA1";

    int arg_table_size = 0;

    std::map<std::string, int> var_table;
    var_table["ZMIENNA1"] = 0;



    auto x = new FunctionPrototype(name, dtt, dtt_size, dtt_args, dtt_args_size, arg_table_size, var_table);
    return x;
}

FunctionPrototype::FunctionPrototype(std::string name, dtt_type* dtt, int dtt_size, dtt_arg* dtt_args, int dtt_args_size,
                                     int arg_table_size, std::map<std::string, int>var_table) {
    this->name = name;
    this->dtt = dtt;
    this->dtt_size = dtt_size;
    this->dtt_args = dtt_args;
    this->dtt_args_size = dtt_args_size;
    this->arg_table_size = arg_table_size;
    this->var_table = var_table;
}

Function* FunctionPrototype::generate() {
    return new Function(*this);
}

Function* FunctionFactory::makeFunction(std::string functionName) {
    auto functionPrototype = FunctionFactory::functionsPrototypes[functionName];
    return functionPrototype->generate();
}



Function::Function(FunctionPrototype& functionPrototype) {
    this->name = functionPrototype.name;
    this->dtt = functionPrototype.dtt;
    this->dtt_size = functionPrototype.dtt_size;

    this->dtt_args = functionPrototype.dtt_args;
    this->dtt_args_size = functionPrototype.dtt_args_size;

    this->arg_table = new int[functionPrototype.arg_table_size];
    this->var_table = functionPrototype.var_table;
}

void Function::run() {
    std::cout << *this << std::endl;
    while(this->vpc < this->dtt_size)
        this->dtt[this->vpc++]();
}

std::ostream& operator<<(std::ostream& s, const Function& function)
{
    s << function.name << "\n";
    s << "CODE:\n";
    for (int i = 0; i < function.dtt_args_size; ++i) {
        s << "  " << bytecodeMapping[function.dtt[i]] << "\n";
    }
    return s;
}  
