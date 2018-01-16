//
// Created by gros on 13.12.17.
//


#include "Function.h"
#include "VM.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <cstring>


const std::string argTypeToStr(unsigned char c) {
    switch(c) {
        case 0:
            return "VAR";
        case 1:
            return "ARG";
        case 2:
            return "CONST";
        case 3:
            return "FUNC";
        case 4:
            return "THREAD";
        default:
            return "UNKNOWN";
    }
}


dtt_arg FunctionFactory::parse_load(std::string line, int arg_table_size, std::map<std::string, int> &var_table) {
    std::string bytecode;
    std::string bytecode_arg;
    dtt_arg dtt_arg_tmp = dtt_arg();

    if(line.find(" ") == line.npos)
        throw ParserException("Argument to LOAD bytecode required");
    bytecode = line.substr(0, line.find(" "));
    bytecode_arg = line.substr(line.find(" ")+1);

    if(bytecode == "LOAD") {
        if(startswith(bytecode_arg, "ARG_")) {
            dtt_arg_tmp.type = ARG;
            try {
                dtt_arg_tmp.valInt = std::stoi(bytecode_arg.substr(4));
            } catch(std::invalid_argument) {
                throw ParserException("Invalid integer after ARG_: " + bytecode_arg);
            }
            if(dtt_arg_tmp.valInt >= arg_table_size)
                throw ParserException("Integer after ARG_ too large");
            if(dtt_arg_tmp.valInt < 0)
                throw ParserException("Integer after ARG_ too small");
        } else {
            dtt_arg_tmp.type = CONST;
            try {
                dtt_arg_tmp.valInt = std::stoi(bytecode_arg);
            } catch(std::invalid_argument) {
                throw ParserException("Invalid const: " + bytecode_arg);
            }
        }
    } else if(bytecode == "LOADV") {
        if(var_table.find(bytecode_arg) == var_table.end())
            throw ParserException("Variable " + bytecode_arg + " not found");
        dtt_arg_tmp.type = VAR;
        dtt_arg_tmp.valStr = bytecode_arg;
    } else if(bytecode == "LOADF") {
        dtt_arg_tmp.type = FUNC;
        dtt_arg_tmp.valStr = bytecode_arg;
    } else if(bytecode == "LOADT") {
        dtt_arg_tmp.type = THREAD;
        dtt_arg_tmp.valStr = bytecode_arg;
    } else {
        throw ParserException("Wrong LOAD bytecode: " + bytecode);
    }
    return dtt_arg_tmp;
}

std::pair<bool, int> FunctionFactory::check_instruction(std::string line, std::vector<dtt_arg> &dtt_args_vector, int arg_counter) {
    std::vector<std::set<unsigned char>> &required_args = bytecodeMapping.at(line).second;
    auto argToCheck = dtt_args_vector.end() - arg_counter;
    bool unknownNumberOfArguments = false;
    int argumentsChecked = 0;

    for(auto required_arg_tuple = required_args.begin(); required_arg_tuple != required_args.end(); ++required_arg_tuple) {
        // empty set -> unknown (at parsing time) number of elements
        if(required_arg_tuple->empty()) {
            unknownNumberOfArguments = true;
            break;
        }
        if(required_arg_tuple->find((*argToCheck).type) == required_arg_tuple->end()) {
            std::string err_msg = "Wrong arguments (calls to LOADs) for " + line + "\n";
            err_msg += "Required: " + vector2string(required_args) + "\n";
            err_msg += "Found: [";
            for (auto it = dtt_args_vector.end() - arg_counter; it != dtt_args_vector.end(); ++it)
                err_msg += argTypeToStr((*it).type) + ", ";
            err_msg += "]";
            throw ParserException(err_msg);
        }
        argToCheck++;
        argumentsChecked++;
    }
    return std::make_pair(unknownNumberOfArguments, argumentsChecked);
}

std::pair<FunctionPrototype*, std::forward_list<std::tuple<std::string, int, int>>> FunctionFactory::parseCode(std::string codePath) {
    bool endReached = false;
    std::string line;
    std::ifstream codeFile(codePath);

    if(!codeFile.is_open())
        throw ParserException("Can't open file: " + codePath);

    // begins with DEF FUNC_NAME ARGS_COUNT
    std::string name;
    int arg_table_size = 0;

    std::getline(codeFile, line);
    trim(line);
    if(!startswith(line, "DEF "))
        throw ParserException("Function must starts with DEF");
    line = line.substr(4);

    if(line.find(" ") == line.npos)
        throw ParserException("Function must have FUNC_NAME after DEF");
    name = line.substr(0, line.find(" "));
    line = line.substr(line.find(" ")+1);

    if(line.size() > 0) {
        try {
            arg_table_size = std::stoi(line);
        } catch(std::invalid_argument) {
            throw ParserException("Incorrect ARGS_COUNT in DEF");
        }
    }

    // DEFINE -  declarations of variables
    std::map<std::string, int> var_table;
    std::string var_name;

    while (std::getline(codeFile, line)) {
        trim(line);
        if(!line.empty() && !startswith(line, "//")) {
            if(startswith(line, "DECLARE ")) {
                var_name = line.substr(8);
                if(line.empty())
                    throw ParserException("VAR_NAME after DEFINE required");
                var_table[var_name] = 0;
            } else
                break;
        }
    }

    // bytecodes
    std::vector<dtt_func> *dtt = new std::vector<dtt_func>;
    std::vector<dtt_arg> dtt_args_vector;
    std::string bytecode;
    std::string bytecode_arg;

    std::forward_list<std::tuple<std::string, int, int>> calledFunctionsToCheck;
    int arg_counter = 0;

    do {
        trim(line);
        if(!line.empty() && !startswith(line, "//")) {
            // PARSE LOADS
            if(startswith(line, "LOAD")) {
                dtt_arg dtt_arg_tmp = parse_load(line, arg_table_size, var_table);
                dtt_args_vector.push_back(dtt_arg_tmp);
                arg_counter++;
            // PARSE END
            } else if(line == "END") {
                endReached = true;
            // PARSE DEFINE
            } else if(startswith(line, "DEFINE")) {
                throw ParserException("Variables definitions can appear only at the beginning og the function");
            // PARSE OTHERS
            } else if(bytecodeMapping.find(line) != bytecodeMapping.end()) {
                auto instructionChecked = check_instruction(line, dtt_args_vector, arg_counter);
                bool unknownNumberOfArguments = instructionChecked.first;
                int argumentsChecked = instructionChecked.second;
                if(unknownNumberOfArguments) {
                    std::string functionToCheckName = dtt_args_vector.at(dtt_args_vector.size() - arg_counter).valStr;
                    calledFunctionsToCheck.push_front(std::make_tuple(functionToCheckName,
                                                                      dtt_args_vector.size() - arg_counter,
                                                                      arg_counter - argumentsChecked));
                }
                arg_counter = 0;
                dtt->push_back(bytecodeMapping.at(line).first);
            } else {
                throw ParserException("Unknown bytecode: " + line);
            }
        }
    } while (!endReached && std::getline(codeFile, line));
    codeFile.close();

    if(!endReached)
        throw ParserException("END not fund");

    std::forward_list<dtt_arg>* dtt_args = new std::forward_list<dtt_arg>(dtt_args_vector.begin(), dtt_args_vector.end());

    return std::make_pair(new FunctionPrototype(name, dtt, dtt_args, arg_table_size, var_table), calledFunctionsToCheck);
}

FunctionFactory::FunctionFactory(std::string codeDirPath) {
    this->initialize(codeDirPath);
}

FunctionFactory::~FunctionFactory() {
    for(auto it = this->functionsPrototypes.begin(); it != this->functionsPrototypes.end(); it++)
        delete (*it).second;
}

void FunctionFactory::initialize(std::string codeDirPath) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (codeDirPath.c_str())) == NULL) {
        throw ParserException("Path not found: " + codeDirPath);

    }

    std::forward_list<std::tuple<std::string, int, int>> calledFunctionsToCheck;
    while ((ent = readdir (dir)) != NULL) {
        std::string filename = ent->d_name;
        if(endswith(filename, bytecodeExtension)) {
            auto parsedCode = parseCode(codeDirPath + filename);
            auto functionPrototype = parsedCode.first;
            calledFunctionsToCheck.merge(parsedCode.second);
            this->addFunction(functionPrototype);
        }
    }
    closedir (dir);

    for(auto it = calledFunctionsToCheck.begin(); it != calledFunctionsToCheck.end(); it++) {
        std::string functionToCheckName = std::get<0>(*it);
//        int functionArgumentsStartPosition = std::get<1>(*it);
        int functionArgumentsCount = std::get<2>(*it);

        if(!this->haveFunction(functionToCheckName))
            throw ParserException("Function " + functionToCheckName + " not found");

        FunctionPrototype* fp = this->functionsPrototypes[functionToCheckName];
        if(fp->argTableSize != functionArgumentsCount) {
            std::string errorMsg = "Function " + functionToCheckName + " called with wrong number of argument";
            errorMsg += "\nHave: " + std::to_string(functionArgumentsCount);
            errorMsg += "\nShould be: " + std::to_string(fp->argTableSize);
            throw ParserException(errorMsg);
        }
    }
}

Function* FunctionFactory::makeFunction(std::string functionName) {
    auto functionPrototype = this->functionsPrototypes[functionName];
    return functionPrototype->generate();
}

void FunctionFactory::addFunction(FunctionPrototype* functionPrototype) {
    this->functionsPrototypes[functionPrototype->name] = functionPrototype;
}

bool FunctionFactory::haveFunction(std::string functionPrototypeName) {
    return this->functionsPrototypes.find(functionPrototypeName) != this->functionsPrototypes.end();
}

void FunctionFactory::setSchedulingFrequency(unsigned long frequency) {
    for(auto it = this->functionsPrototypes.begin(); it != this->functionsPrototypes.end(); it++)
            (*it).second->maxBlockSize = frequency;
}


FunctionPrototype::FunctionPrototype(std::string name, std::vector<dtt_func>* dtt,
                                     std::forward_list<dtt_arg>* dtt_args, int arg_table_size,
                                     std::map<std::string,int>var_table) {
    this->name = name;
    this->dtt = dtt;
    this->dttArgs = dtt_args;
    this->argTableSize = arg_table_size;
    this->varTable = var_table;
    this->jit = new std::map<unsigned long, jit_func>;
}

FunctionPrototype::~FunctionPrototype() {
    delete this->dtt;
    delete this->dttArgs;
    delete this->jit;
    this->dtt = nullptr;
    this->dttArgs = nullptr;
    this->jit = nullptr;
}

Function* FunctionPrototype::generate() {
    return new Function(*this);
}

Function::Function(FunctionPrototype& functionPrototype) {
    this->name = functionPrototype.name;
    this->dtt = functionPrototype.dtt;
    this->dttArgs = new std::forward_list<dtt_arg>(functionPrototype.dttArgs->begin(), functionPrototype.dttArgs->end());

    this->argTableSize = functionPrototype.argTableSize;
    this->varTable = functionPrototype.varTable;

    this->vpc = 0;

    this->anotherFunctionCalled = false;
    this->blocked = false;
    this->waiting = false;
    this->returnFunction = nullptr;
    this->returnVariable = "";

    this->jit = functionPrototype.jit;
    this->maxBlockSizePtr = &functionPrototype.maxBlockSize;
}

Function::~Function() {
    delete this->dttArgs;
    this->dttArgs = nullptr;
}

void Function::run() {
    jit_func compiled;
    if(this->jit->find(this->vpc) != this->jit->end()) {
        compiled = this->jit->at(this->vpc);
    } else {
        compiled = this->compile();
        this->jit->emplace(this->vpc, compiled);
    }
    compiled(&VM::getVM());
//    while(this->vpc != this->dtt->end() && !this->anotherFunctionCalled && !this->blocked && !this->waiting)
//        (*this->vpc)();
}

jit_func Function::compile() {
    auto vm = VM::getVM();
    std::string blocksDir = vm.blocksDir;
    std::string blockName = this->name + "_block_" + std::to_string(this->vpc);

    // compile only if rebuild option is true or it's not already compiled
    struct stat buffer;
    if(vm.rebuild || stat((blocksDir + blockName + ".so").c_str(), &buffer) != 0) {

        // prepare blocks as string with cpp code to compile
        std::array<dtt_func, 3> blockingBytecodes = {vm_call, vm_join, vm_recv};
        unsigned long blockCounter = 0;
        std::string blocks = vm_prolog(blockName);
        bool nextIsBlockingBytecode = false;

        nextIsBlockingBytecode = std::find(blockingBytecodes.begin(), blockingBytecodes.end(),
                                           dtt->at(this->vpc + blockCounter)) != blockingBytecodes.end();
        if(nextIsBlockingBytecode)
            blocks += dtt->at(this->vpc + blockCounter)();

        while((*this->maxBlockSizePtr == 0 || blockCounter < *this->maxBlockSizePtr) &&
                this->vpc + blockCounter < this->dtt->size() &&
                std::find(blockingBytecodes.begin(), blockingBytecodes.end(),
                          dtt->at(this->vpc + blockCounter)) == blockingBytecodes.end()) {
            blocks += dtt->at(this->vpc + blockCounter)();
            blockCounter++;
        }

        blocks += vm_epilog();

        // prepare directory
        if(mkdir(blocksDir.c_str(), S_IRWXU) != 0 && errno != EEXIST)
            throw VMRuntimeException(strerror(errno));

        // save blocks to file
        std::ofstream(blocksDir + blockName + ".cpp") << blocks;

        // run external compiler
        auto compilerCommand = "g++ -g -c -fPIC -o " + blocksDir + blockName + ".o  " + blocksDir + blockName + ".cpp";
        vm.print("Compiling with: " + compilerCommand);
        if(std::system(compilerCommand.c_str()) != 0)
            throw VMRuntimeException("Compiler error");

        compilerCommand = "g++ -g -shared -o " + blocksDir + blockName + ".so  " + blocksDir + blockName + ".o";
        vm.print("Compiling with: " + compilerCommand);
        if(std::system(compilerCommand.c_str()) != 0)
            throw VMRuntimeException("Compiler error");
    }

    // load dynamic library
    void* handle = dlopen((blocksDir + blockName + ".so").c_str(), RTLD_NOW | RTLD_NODELETE);
    if (!handle)
        throw VMRuntimeException(std::string("Cannot open library: ") + dlerror());

    dlerror();  // reset
    jit_func compiled = (jit_func) dlsym(handle, blockName.c_str());
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
        dlclose(handle);
        throw VMRuntimeException(std::string("Cannot load symbol 'hello': ") + dlsym_error);
    }

    dlclose(handle);
    return compiled;
}

dtt_arg& Function::getNextArg(bool increment) {
    dtt_arg& nextArg = this->dttArgs->front();
    if(increment)
        this->dttArgs->pop_front();
    return nextArg;
}

std::vector<std::string> Function::toStr() const {
    std::vector<std::string> s;
    s.push_back(this->name);
    s.push_back("CODE:");
    for (auto bytecode = this->dtt->begin(); bytecode != this->dtt->end(); bytecode++) {
        for (auto it=bytecodeMapping.begin(); it!=bytecodeMapping.end(); ++it)
            if(it->second.first == *bytecode) {
                s.push_back("    " + it->first);
                break;
            }
    }
    return s;
}

std::ostream& operator<<(std::ostream& s, const Function& function) {
    auto lines = function.toStr();
    for(auto it = lines.begin(); it != lines.end(); it++)
        s << *it << "\n";
    return s;
}

void Function::setArguments(std::vector<int> arguments) {
    for (auto it = this->dttArgs->begin(); it != this->dttArgs->end(); it++) {
        if((*it).type == ARG) {
            (*it).type = CONST;
            (*it).valInt = arguments.at((*it).valInt);
        }
    }
}


std::string vm_prolog(std::string blockName) {
    std::string prolog = R"END(
#include <iostream>
#include <vector>
#include <map>
#include <forward_list>

const unsigned char VAR = 0;
const unsigned char ARG = 1;
const unsigned char CONST = 2;
const unsigned char FUNC = 3;
const unsigned char THREAD = 4;

typedef void (*jit_func)(void*);
typedef std::string (*dtt_func)();


typedef struct dtt_arg {
    unsigned char type;
    int valInt;
    std::string valStr;
} dtt_arg;


class Function;

class FunctionPrototype {
public:
    FunctionPrototype(std::string, std::vector<dtt_func>*, std::forward_list<dtt_arg>*, int, std::map<std::string, int>);
    ~FunctionPrototype();
    Function* generate();

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
    dtt_arg& getNextArg(bool = true);
    void setArguments(std::vector<int>);
    std::vector<std::string> toStr() const;
    friend std::ostream& operator<<(std::ostream&, const Function&);

    std::string name;
    std::vector<dtt_func> *dtt;
    std::forward_list<dtt_arg> *dttArgs;
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

class Thread {
public:
    Thread(std::string, Function*);
    ~Thread();
    void run();

    void joining(Thread*);
    void unblock();
    void receive(int);

    std::string name;
    Function* currect_function;
    unsigned char status;
    std::vector<int> recv_table;
    std::vector<Thread*> joiningThreads;
    bool reshedule;
};

class VM {
public:
    void checkAllThreadsWaiting();

    Function* getCurrentFunction();
    Function* getNewFunction(std::string);

    Thread* getNewThread(std::string, std::string);
    Thread* getCurrentThread();
    void stopThread(std::string);
    Thread* getThread(std::string);

    bool changeScheduler(std::string);
    void setSchedulingFrequency(int);

    void print(std::string);

#if DEBUG == 1
    std::vector<WINDOW*> windows;
    WINDOW* terminal;
    void refresh();
#endif
};

    )END";

    prolog += "extern \"C\" void ";
    prolog += blockName + "(VM &vm) {\n";
    prolog += "    auto currentFunction = vm.getCurrentFunction();\n";
    return prolog;
}

std::string vm_epilog() {
    return R"END(
}
    )END";
}

std::string vm_assign(){
    return R"END({
    // vm_assign
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable[arg1.valStr];

    currentFunction->varTable[arg0.valStr] = val1;
    currentFunction->vpc++;
    })END";
};
std::string vm_print() {
    return R"END({
    // vm_print
    auto arg0 = currentFunction->getNextArg();

    int val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction->varTable[arg0.valStr];

    vm.print(std::to_string(val0));
    currentFunction->vpc++;
    })END";
}

std::string vm_call(){
    return R"END({
    // vm_call
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    currentFunction->anotherFunctionCalled = true;
    currentFunction->returnVariable = arg1.valStr;

    auto newFunction = vm.getNewFunction(arg0.valStr);
    std::vector<int> newFunctionArgs;

    for (int i = 0; i < newFunction->argTableSize; ++i) {
        auto valArg = currentFunction->getNextArg();
        int valNext = valArg.valInt;
        if(valArg.type == VAR)
            valNext = currentFunction->varTable[valArg.valStr];
        newFunctionArgs.push_back(valNext);
    }

    newFunction->setArguments(newFunctionArgs);
    newFunction->returnFunction = currentFunction;

    vm.getCurrentThread()->currect_function = newFunction;
    currentFunction->vpc++;
    return;
    })END";
};
std::string vm_return(){
    return R"END({
    // vm_return
    auto arg0 = currentFunction->getNextArg();

    int val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction->varTable[arg0.valStr];

    if(currentFunction->returnFunction != nullptr) {
        currentFunction->returnFunction->varTable[currentFunction->returnFunction->returnVariable] = val0;
        currentFunction->returnFunction->anotherFunctionCalled = false;
    }
    vm.getCurrentThread()->currect_function = currentFunction->returnFunction;
    delete currentFunction;
    })END";
};

std::string vm_send(){
    return R"END({
    // vm_send
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable[arg1.valStr];

    vm.getThread(arg0.valStr)->receive(val1);
    currentFunction->vpc++;
    })END";
};
std::string vm_recv(){
    return R"END({
    // vm_recv
    auto arg0 = currentFunction->getNextArg(false);
    auto currentThread = vm.getCurrentThread();

    auto recv_table = &currentThread->recv_table;
    if(recv_table->size() != 0) {
        currentFunction->vpc++;
        currentFunction->getNextArg();

        currentFunction->varTable[arg0.valStr] = recv_table->back();
        recv_table->pop_back();

        currentFunction->waiting = false;
    } else {
        vm.checkAllThreadsWaiting();
        currentFunction->waiting = true;
        return;
    }
    })END";
};
std::string vm_start(){
    return R"END({
    // vm_start
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    auto newThread = vm.getNewThread(arg1.valStr, arg0.valStr);

    std::vector<int> newFunctionArgs;
    for (int i = 0; i < newThread->currect_function->argTableSize; ++i) {
        auto valArg = currentFunction->getNextArg();
        int valNext = valArg.valInt;
        if(valArg.type == VAR)
            valNext = currentFunction->varTable[valArg.valStr];
        newFunctionArgs.push_back(valNext);
    }

    newThread->currect_function->setArguments(newFunctionArgs);
    currentFunction->vpc++;
    })END";
};
std::string vm_join(){
    return R"END({
    // vm_join
    auto arg0 = currentFunction->getNextArg(false);

    auto threadToJoin = vm.getThread(arg0.valStr);
    if(threadToJoin != nullptr) {
        currentFunction->blocked = true;
        threadToJoin->joining(vm.getCurrentThread());
        return;
    } else {
        currentFunction->blocked = false;
        currentFunction->vpc++;
        currentFunction->getNextArg();
    }
    })END";
};
std::string vm_stop(){
    return R"END({
    // vm_stop
    auto arg0 = currentFunction->getNextArg();

    vm.stopThread(arg0.valStr);
    currentFunction->vpc++;
    })END";
};

std::string vm_add() {
    return R"END({
    // vm_add
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable[arg2.valStr];

    currentFunction->varTable[arg0.valStr] = val1 + val2;
    currentFunction->vpc++;
    })END";
}
std::string vm_sub() {
    return R"END({
    // vm_sub
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable[arg2.valStr];

    currentFunction->varTable[arg0.valStr] = val1 - val2;
    currentFunction->vpc++;
    })END";
};
std::string vm_div() {
    return R"END({
    // vm_div
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable[arg2.valStr];

    currentFunction->varTable[arg0.valStr] = val1 / val2;
    currentFunction->vpc++;
    })END";
};
std::string vm_mul() {
    return R"END({
    // vm_mul
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->varTable[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->varTable[arg2.valStr];

    currentFunction->varTable[arg0.valStr] = val1 * val2;
    currentFunction->vpc++;
    })END";
};

