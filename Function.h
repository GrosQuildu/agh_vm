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
    char type;
    int valInt;
    std::string valStr;
} dtt_arg_type;

const char VAR = 0;
const char ARG = 1;
const char CONST = 2;
const char FUNC = 3;
const char THREAD = 4;

const std::string const2str(char);

void vm_assign();
void vm_print();
void vm_call();
void vm_return();
void vm_send();
void vm_recv();
void vm_start();
void vm_join();
void vm_stop();
void vm_add();
void vm_sub();
void vm_div();
void vm_mul();


static const std::string bytecodeExtension = ".pp";
static std::map<std::string, std::pair<dtt_type, std::vector<std::set<char>>>> bytecodeMapping = {
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
    dtt_arg_type& getNextArg();
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
     * @return dtt_arg_type - argument for bytecode
     */
    static dtt_arg_type parse_load(std::string, int, std::map<std::string, int>&);


    /**
     * Check if correct arguments are passed to bytecode, returns corresponding function
     * @param string line - bytecode to parse
     * @param vector<dtt_arg_type> dtt_args_vector - arguments loaded
     * @return dtt_type - pointer to function
     */
    static dtt_type parse_instruction(std::string, std::vector<dtt_arg_type>&);

    /**
     * Parse bytecode to FunctionPrototype
     * @param string codePath - path to function's code
     * @return FunctionPrototype*, created with:
         * string name - function name
         * dtt_type[] dtt - dtt table (array of pointers to functions that gets void and returns void)
         * int dtt_size
         * dtt_arg[] dtt_args - table of dtt_arg structs, arguments for functions stored in dtt table
         * int dtt_args_size
         * int arg_table_size - function's arguments table size
         * map<string, int> var_table - function's variables map
     */
    static FunctionPrototype* parseCode(std::string);
};

#endif //VM_FUNCTION_H
