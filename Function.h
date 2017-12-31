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
typedef void (*dtt_func)(Function&);

typedef struct dtt_arg {
    char type;
    int valInt;
    std::string valStr;
} dtt_arg;

const char VAR = 0;
const char ARG = 1;
const char CONST = 2;
const char FUNC = 3;
const char THREAD = 4;

const std::string const2str(char);

void vm_assign(Function &currentFunction);
void vm_print(Function &currentFunction);
void vm_call(Function &currentFunction);
void vm_return(Function &currentFunction);
void vm_send(Function &currentFunction);
void vm_recv(Function &currentFunction);
void vm_start(Function &currentFunction);
void vm_join(Function &currentFunction);
void vm_stop(Function &currentFunction);
void vm_add(Function &currentFunction);
void vm_sub(Function &currentFunction);
void vm_div(Function &currentFunction);
void vm_mul(Function &currentFunction);


static const std::string bytecodeExtension = ".pp";
static std::map<std::string, std::pair<dtt_func, std::vector<std::set<char>>>> bytecodeMapping = {
        // BYTECODE: (function, vector({arg1type | arg1type}, {arg2type}, {arg3type}))
        {"DECLARE", {nullptr,   { {VAR} }}},
        {"ASSIGN",  {vm_assign, { {VAR}, {VAR,CONST,ARG} }}},
        {"PRINT",   {vm_print,  { {VAR,CONST,ARG} }}},

        {"CALL",    {vm_call,   { {FUNC}, {VAR}, {} }}},
        {"RETURN",  {vm_return, { {VAR,CONST,ARG} }}},

        {"SEND",    {vm_send,   { {THREAD}, {VAR,CONST,ARG} }}},
        {"RECV",    {vm_recv,   { {VAR} }}},
        {"START",   {vm_start,  { {FUNC}, {THREAD}, {} }}},
        {"JOIN",    {vm_join,   { {THREAD} }}},
        {"STOP",    {vm_stop,   { {THREAD} }}},

        {"ADD",     {vm_add,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"SUB",     {vm_sub,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"DIV",     {vm_div,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
        {"MUL",     {vm_mul,    { {VAR}, {VAR,CONST,ARG}, {VAR,CONST,ARG} }}},
};


class FunctionPrototype {
public:
    FunctionPrototype(std::string, std::forward_list<dtt_func>*, std::forward_list<dtt_arg>*, int, std::map<std::string, int>);
    Function* generate();

    std::string name;
    std::forward_list<dtt_func> *dtt = nullptr;
    std::forward_list<dtt_arg> *dtt_args = nullptr;
    int arg_table_size = 0;
    std::map<std::string, int> var_table;
};



class Function {
public:
    Function(FunctionPrototype&);

    void run();
    dtt_arg& getNextArg();
    friend std::ostream& operator<<(std::ostream&, const Function&);

    std::string name;
    std::forward_list<dtt_func> *dtt = nullptr;
    std::forward_list<dtt_arg> *dtt_args = nullptr;
    int *arg_table = nullptr;
    std::map<std::string, int> var_table;

    Function* returnFunction = nullptr;
    std::forward_list<dtt_func>::iterator vpc;
    int arg_c = 0;
    int return_variable = 0;
};



class FunctionFactory {
public:
    /**
     * Initialize function factory with function from files in given directory
     * @param string codeDirPath
     */
    void initialize(std::string);

    /**
     * Add function to FunctionFactory from file
     * @param string codePath
     */
    void addFunction(std::string);

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
     * @param vector<dtt_arg> dtt_args_vector - arguments loaded
     * @return dtt_func - pointer to function
     */
    static dtt_func parse_instruction(std::string, std::vector<dtt_arg>&);

    /**
     * Parse bytecode to FunctionPrototype
     * @param string codePath - path to function's code
     * @return FunctionPrototype*, created with:
         * string name - function name
         * std::forward_list<dtt_func> dtt - dtt list (list of pointers to functions that gets void and returns void)
         * std::forward_list<dtt_arg> dtt_args - list of dtt_arg structs, arguments for functions stored in dtt table
         * int arg_table_size - function's arguments table size
         * map<string, int> var_table - function's variables map
     */
    static FunctionPrototype* parseCode(std::string);
};

#endif //VM_FUNCTION_H
