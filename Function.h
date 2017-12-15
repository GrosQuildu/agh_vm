//
// Created by gros on 13.12.17.
//

#ifndef VM_FUNCTION_H
#define VM_FUNCTION_H

#include <map>
#include <string>
#include <netinet/in.h>
#include <vector>
#include <set>

typedef void (* dtt_type)();

typedef struct dtt_arg_type {
    int type;
    int valInt;
    std::string valStr;
} dtt_arg_type;



const char VAR = 0;
const char ARG = 1;
const char CONST = 2;
const char FUNC = 3;
const char THREAD = 4;

void vm_add();
void vm_print();
void vm_return();

static const std::string bytecodeExtension = ".pp";
static std::map<std::string, std::pair<dtt_type, std::vector<std::set<char>>>> bytecodeMapping = {
        // BYTECODE: (function, vector({arg1type | arg1type}, {arg2type}, {arg3type}))
        {"ADD", {vm_add, { {VAR}, {VAR,CONST}, {VAR,CONST} }}},
        {"PRINT", {vm_print, { {VAR,CONST} }}},
        {"RETURN", {vm_return, { {VAR,CONST} }}},
};

class Function;

class FunctionPrototype {
public:
    FunctionPrototype(std::string, dtt_type[], int, dtt_arg_type[], int, int, std::map<std::string, int>);

    Function* generate();

    std::string name;
    dtt_type *dtt = nullptr;
    int dtt_size = 0;
    dtt_arg_type* dtt_args;
    int dtt_args_size = 0;
    int arg_table_size = 0;
    std::map<std::string, int>var_table;
};



class Function {
public:
    Function(FunctionPrototype&);

    void run();
    friend std::ostream& operator<<(std::ostream&, const Function&);

    std::string name;
    dtt_type *dtt = nullptr;
    int dtt_size = 0;
    dtt_arg_type* dtt_args;
    int dtt_args_size = 0;
    int *arg_table;
    std::map<std::string, int>var_table;

    Function* returnFunction = nullptr;
    int vpc = 0;
    int arg_c = 0;
    int return_variable = 0;
};



class FunctionFactory {
public:
    void initialize(std::string);
    void addFunction(std::string);
    bool haveFunction(std::string);
    Function* makeFunction(std::string);  // factory method

private:
    std::map<std::string, FunctionPrototype*> functionsPrototypes;
    FunctionPrototype* parseCode(std::string);
};

#endif //VM_FUNCTION_H
