//
// Created by gros on 13.12.17.
//

#ifndef VM_FUNCTION_H
#define VM_FUNCTION_H

#include <map>
#include <string>
#include <netinet/in.h>

typedef void (* dtt_type)();

typedef struct dtt_arg {
    int type;
    int valInt;
    std::string valStr;
} dtt_arg;

const int VAR = 0;
const int ARG = 1;
const int CONST = 2;

void vm_add();
void vm_print();
void vm_return();


class Function;

class FunctionPrototype {
public:
    FunctionPrototype(std::string, dtt_type[], int, dtt_arg[], int, int, std::map<std::string, int>);

    Function* generate();

    std::string name;
    dtt_type *dtt = nullptr;
    int dtt_size = 0;
    dtt_arg* dtt_args;
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
    dtt_arg* dtt_args;
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
    Function* makeFunction(std::string);

private:
    std::map<std::string, FunctionPrototype*> functionsPrototypes;
    FunctionPrototype* parseCode(std::string);
};

#endif //VM_FUNCTION_H
