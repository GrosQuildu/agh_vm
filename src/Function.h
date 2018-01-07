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
#include <forward_list>

#include "Exceptions.h"
#include "Helpers.h"

#include <dirent.h>
#include <sstream>
#include <iostream>
#include <fstream>


class Function;
typedef void (*dtt_func)();

typedef struct dtt_arg {
    unsigned char type;
    int valInt;
    std::string valStr;
} dtt_arg;

const unsigned char VAR = 0;
const unsigned char ARG = 1;
const unsigned char CONST = 2;
const unsigned char FUNC = 3;
const unsigned char THREAD = 4;

const std::string const2str(unsigned char);

std::string vm_prolog();
std::string vm_epilog();
void vm_schedule();
std::string vm_assign();
std::string vm_print();
void vm_call();
void vm_return();
void vm_send();
void vm_recv();
void vm_start();
void vm_join();
void vm_stop();
std::string vm_add();
void vm_sub();
void vm_div();
void vm_mul();


static const std::string bytecodeExtension = ".pp";
static std::map<std::string, std::pair<unsigned char, std::vector<std::set<unsigned char>>>> bytecodeMapping = {
        // BYTECODE: (function, vector({arg1type | arg1type}, {arg2type}, {arg3type}))
        {"DECLARE", {-1,   { {VAR} }}},
        {"ASSIGN",  {1, { {VAR}, {VAR,CONST,ARG} }}},
        {"PRINT",   {2,  { {VAR,CONST,ARG} }}},

        {"CALL",    {2,   { {FUNC}, {VAR}, {} }}},
        {"RETURN",  {3, { {VAR,CONST,ARG} }}},

        {"SEND",    {4,   { {THREAD}, {VAR,CONST,ARG} }}},
        {"RECV",    {5,   { {VAR} }}},
        {"START",   {6,  { {FUNC}, {THREAD}, {} }}},
        {"JOIN",    {7,   { {THREAD} }}},
        {"STOP",    {8,   { {THREAD} }}},

        {"ADD",     {3,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"SUB",     {10,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"DIV",     {11,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"MUL",     {12,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
};


class FunctionPrototype {
public:
    FunctionPrototype(std::string, std::vector<unsigned char>*, std::forward_list<dtt_arg>*, int, std::map<std::string, int>);
    ~FunctionPrototype();
    Function* generate();

    std::string name;
    std::vector<unsigned char> *dtt;
    std::forward_list<dtt_arg> *dtt_args;
    int arg_table_size;
    std::map<std::string, int> var_table;

    std::map<unsigned long, dtt_func> *jit;
    unsigned long maxInstructionsInBlock;
};



class Function {
public:
    Function(FunctionPrototype&);
    ~Function();

    void run();
    dtt_arg* getNextArg();
    void setArguments(std::vector<int>);
    dtt_func compile();
    std::vector<std::string> toStr() const;
    friend std::ostream& operator<<(std::ostream&, const Function&);

    std::string name;
    std::vector<unsigned char> *dtt;
    std::forward_list<dtt_arg> *dtt_args;
    int arg_table_size;
    std::map<std::string, int> var_table;

    unsigned long vpc;

    bool anotherFunctionCalled;
    Function* returnFunction;
    std::string return_variable;

    std::map<unsigned long, dtt_func> *jit;
    unsigned  long *maxInstructionsInBlock;
};



class FunctionFactory {
public:
    FunctionFactory() {};
    FunctionFactory(std::string);
    ~FunctionFactory();

    /**
     * Initialize function factory with function from files in given directory
     * @param string codeDirPath
     */
    void initialize(std::string);

    /**
     * Add functionPrototype to FunctionFactory
     * @param FunctionPrototype* functionPrototype
     */
    void addFunction(FunctionPrototype*);

    /**
     * Check if function with given name exists in factory
     * @param string functionPrototypeName
     * @return bool
     */
    bool haveFunction(std::string);

    /**
     * Make function from prototype
     * @param string functionName
     * @return Function*
     */
    Function* makeFunction(std::string);  // factory method

    void setMaxBlockSize(unsigned long);

private:
    std::map<std::string, FunctionPrototype*> functionsPrototypes;

    /**
     * Parse load bytecode (loads argument for bytecode), returns the argument
     * @param string line - bytecode to parse
     * @param int arg_table_size
     * @param map<std::string, int> var_table
     * @return dtt_arg - argument for bytecode
     */
    static dtt_arg parse_load(std::string, int, std::map<std::string, int>&);


    /**
     * Check if correct arguments are passed to bytecode, returns corresponding function
     * @param string line - bytecode to parse
     * @param vector<dtt_arg> dtt_args_vector - all arguments loaded so far
     * @return dtt_func - pointer to function
     */
    static std::pair<bool, int> check_instruction(std::string, std::vector<dtt_arg>&, int);

    /**
     * Parse bytecode to FunctionPrototype
     * @param string codePath - path to function's code
     * @return pair<FunctionPrototype*, forward_list<string, int>>
        * FunctionPrototype* created with:
            * string name - function name
            * std::forward_list<dtt_func> dtt - dtt list (list of pointers to functions that gets void and returns void)
            * std::forward_list<dtt_arg> dtt_args - list of dtt_arg structs, arguments for functions stored in dtt table
            * int arg_table_size - function's arguments table size
            * map<string, int> var_table - function's variables map
        * forward_list<string, int> calledFunctionsToCheck with:
            * function's name
            * number of argument the function will be called
     */
    static std::pair<FunctionPrototype*, std::forward_list<std::tuple<std::string, int, int>>> parseCode(std::string);
};

#endif //VM_FUNCTION_H
