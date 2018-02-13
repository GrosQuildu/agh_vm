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
#include "Bytecode.h"

#include <dirent.h>
#include <sstream>
#include <iostream>
#include <fstream>

typedef void (*jit_func)(void*);
typedef std::string (*dtt_func)();

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

const std::string argTypeToStr(unsigned char);

static const std::string bytecodeExtension = ".pp";
static std::map<std::string, std::pair<dtt_func, std::vector<std::set<unsigned char>>>> bytecodeMapping = {
        // BYTECODE: (function, vector({arg1type | arg1type}, {arg2type}, {arg3type}))
        {"DECLARE",  {nullptr,     { {VAR} }}},
        {"ASSIGN",   {vm_assign,   { {VAR}, {VAR,CONST,ARG} }}},
        {"PRINT",    {vm_print,    { {VAR,CONST,ARG} }}},

        {"CALL",     {vm_call,     { {FUNC}, {VAR}, {} }}},
        {"RETURN",   {vm_return,   { {VAR,CONST,ARG} }}},

        {"SEND",     {vm_send,     { {THREAD}, {VAR,CONST,ARG} }}},
        {"RECV",     {vm_recv,     { {VAR} }}},
        {"START",    {vm_start,    { {FUNC}, {THREAD}, {} }}},
        {"JOIN",     {vm_join,     { {THREAD} }}},
        {"STOP",     {vm_stop,     { {THREAD} }}},
        {"PRIORITY", {vm_priority, { {THREAD}, {VAR,CONST,ARG} }}},

        {"ADD",      {vm_add,      { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"SUB",      {vm_sub,      { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"DIV",      {vm_div,      { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"MUL",      {vm_mul,      { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
};


class Function;

class FunctionPrototype {
public:
    FunctionPrototype(std::string, std::vector<dtt_func>*, std::forward_list<dtt_arg>*, int, std::map<std::string, int>);
    ~FunctionPrototype();
    Function* generate();  // prototype

    std::string name;
    std::vector<dtt_func> *dtt;
    std::forward_list<dtt_arg> *dttArgs;
    int argTableSize;
    std::map<std::string, int> varTable;
    std::map<unsigned long, jit_func> *jit;
    unsigned long maxBlockSize;
};



class Function {
public:
    Function(FunctionPrototype&);
    ~Function();

    void run();
    jit_func compile();
    dtt_arg getNextArg(bool = true);
    void setArguments(std::vector<int>);
    std::vector<std::string> toStr() const;
    friend std::ostream& operator<<(std::ostream&, const Function&);

    std::string name;
    std::vector<dtt_func> *dtt;
    std::forward_list<dtt_arg> *dttArgs;
    std::forward_list<dtt_arg>::iterator dttArgsIt;
    int argTableSize;
    std::map<std::string, int> varTable;
    unsigned long vpc;

    std::map<unsigned long, jit_func> *jit;
    unsigned long *maxBlockSizePtr;

    bool anotherFunctionCalled;
    bool blocked;
    bool waiting;
    Function* returnFunction;
    std::string returnVariable;
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
    Function* makeFunction(std::string);  // factory

    void setSchedulingFrequency(unsigned long);

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
