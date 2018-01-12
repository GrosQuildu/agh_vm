//
// Created by gros on 13.12.17.
//

#include "Function.h"
#include "VM.h"
#include <sys/mman.h>
#include <cstring>


const std::string const2str(unsigned char c) {
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


/* FunctionFactory Start */
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
                err_msg += const2str((*it).type) + ", ";
            err_msg += "]";
            throw ParserException(err_msg);
        }
        argToCheck++;
        argumentsChecked++;
    }
    return std::make_pair(unknownNumberOfArguments, argumentsChecked);
}

std::pair<FunctionPrototype*, std::forward_list<std::tuple<std::string, int, int>>>
FunctionFactory::parseCode(std::string codePath) {
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
    std::vector<unsigned char> *dtt = new std::vector<unsigned char>;
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
            } else if(startswith(line, "DECLARE")) {
                throw ParserException("Variables definitions can appear only at the beginning of the function");
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
        if(fp->arg_table_size != functionArgumentsCount) {
            std::string errorMsg = "Function " + functionToCheckName + " called with wrong number of argument";
            errorMsg += "\nHave: " + std::to_string(functionArgumentsCount);
            errorMsg += "\nShould be: " + std::to_string(fp->arg_table_size);
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

void FunctionFactory::setMaxBlockSize(unsigned long maxBlockSize) {
    for(auto it = this->functionsPrototypes.begin(); it != this->functionsPrototypes.end(); it++) {
        (*it).second->maxInstructionsInBlock = maxBlockSize;
    }
}
/* FunctionFactory End*/


/* FunctionPrototype Start */
FunctionPrototype::FunctionPrototype(std::string name, std::vector<unsigned char>* dtt,
                                     std::forward_list<dtt_arg>* dtt_args, int arg_table_size,
                                     std::map<std::string,int>var_table) {
    this->name = name;
    this->dtt = dtt;
    this->dtt_args = dtt_args;
    this->arg_table_size = arg_table_size;
    this->var_table = var_table;
    this->jit = new std::map<unsigned long, void(*)()>;
    this->maxInstructionsInBlock = 0;
}

FunctionPrototype::~FunctionPrototype() {
    delete this->dtt;
    delete this->dtt_args;
    delete this->jit;

    this->dtt = nullptr;
    this->dtt_args = nullptr;
    this->jit = nullptr;
}

Function* FunctionPrototype::generate() {
    return new Function(*this);
}
/* FunctionPrototype End*/


/* Function Start */
Function::Function(FunctionPrototype& functionPrototype) {
    this->name = functionPrototype.name;
    this->dtt = functionPrototype.dtt;
    this->dtt_args = new std::forward_list<dtt_arg>(functionPrototype.dtt_args->begin(), functionPrototype.dtt_args->end());

    this->arg_table_size = functionPrototype.arg_table_size;
    this->var_table = functionPrototype.var_table;

    this->vpc = 0;

    this->anotherFunctionCalled = false;
    this->returnFunction = nullptr;
    this->return_variable = "";

    this->jit = functionPrototype.jit;
    this->maxInstructionsInBlock = &functionPrototype.maxInstructionsInBlock;
}

Function::~Function() {
    delete this->dtt_args;
    this->dtt_args = nullptr;
    this->maxInstructionsInBlock = nullptr;
}

void Function::run() {
//    if(this->jit->find(this->vpc) != this->jit->end()) {
//        this->jit->at(this->vpc)();
//    } else {
//        dtt_func compiled = this->compile();
//        compiled();
//    }
//    while(this->vpc < this->dtt->si && !this->anotherFunctionCalled)
//        this->dtt[this->vpc++]();

//    typedef std::string (*bc)();
    void*bytecodes[] = {&&prolog, &&assign, &&print, &&add, &&epilog};

    void* startCompiledBlock = (void *)mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    size_t size = (char*)&&epilog - (char*)&&prolog;
    memcpy(startCompiledBlock, &&prolog, size);
    ((dtt_func)startCompiledBlock)();

    prolog:
    auto getCurrentFunction = VM::getCurrentFunction;
    auto getCurrentFunctionNextArgInt = VM::getCurrentFunctionNextArgInt;
    auto getCurrentFunctionNextArgStr = VM::getCurrentFunctionNextArgStr;
    auto setVarTable = VM::setVarTable;
    auto print = VM::print;

    dtt_arg *arg0;
    dtt_arg *arg1;
    dtt_arg *arg2;
    int val0, val1, val2;

    assign:
    setVarTable(getCurrentFunctionNextArgStr(), getCurrentFunctionNextArgInt());

    print:
    print(std::to_string(getCurrentFunctionNextArgInt()));

    add:
    setVarTable(getCurrentFunctionNextArgStr(), getCurrentFunctionNextArgInt() + getCurrentFunctionNextArgInt());

    epilog:
    return;
}

dtt_arg* Function::getNextArg() {
    dtt_arg* nextArg = &this->dtt_args->front();
    this->dtt_args->pop_front();
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
    for (auto it = this->dtt_args->begin(); it != this->dtt_args->end(); it++) {
        if((*it).type == ARG) {
            (*it).type = CONST;
            (*it).valInt = arguments.at((*it).valInt);
        }
    }
}

dtt_func Function::compile() {
    std::string block;
    unsigned long instructionsCompiled = 0;
    typedef std::string (*bc)();
    bc bytecodes[] = {vm_prolog, vm_assign, vm_print, vm_add, vm_epilog};

    block += bytecodes[0]();

    while(this->vpc < this->dtt->size()) {
        if(*this->maxInstructionsInBlock != 0 && instructionsCompiled > *this->maxInstructionsInBlock)
            break;
        block += bytecodes[this->dtt->at(this->vpc)]();
        this->vpc++;
        instructionsCompiled++;
    }

    char* startCompiledBlock = (char *)mmap(NULL, block.size(), PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(startCompiledBlock == MAP_FAILED)
        throw VMRuntimeException("Can't mmap memory, error: " + std::string(strerror(errno)));

    memcpy(startCompiledBlock, block.c_str(), block.size());

    int mprotectSuccess = mprotect(startCompiledBlock, block.size(), PROT_READ | PROT_EXEC);
    if(mprotectSuccess != 0)
        throw VMRuntimeException("Can't mprotect memory, error: " + std::string(strerror(errno)));

    this->jit->insert(std::make_pair(this->vpc, (dtt_func)startCompiledBlock));
    return (dtt_func)startCompiledBlock;
}

/*
dtt_func Function::compile() {
    const unsigned int maxBlockLength = 4096;
    char* startCompiledBlock = (char *)mmap(NULL, maxBlockLength, PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(startCompiledBlock == MAP_FAILED)
        throw VMRuntimeException("Can't mmap memory, error: " + std::string(strerror(errno)));

    unsigned long instructionsCompiled = 0;
    char* endCompiledBlock = startCompiledBlock;
    typedef std::string (*bc)();
    bc bytecodes[] = {vm_prolog, vm_assign, vm_print, vm_add, vm_epilog};


    std::string codePart = bytecodes[0]();
    auto toAddSize = codePart.size();
    memcpy(endCompiledBlock, codePart.c_str(), toAddSize);
    endCompiledBlock += toAddSize;

    while(this->vpc < this->dtt->size()) {
        if(*this->maxInstructionsInBlock != 0 && instructionsCompiled > *this->maxInstructionsInBlock)
            break;
//        size_t toAddSize = (char*)bytecodes[this->dtt->at(this->vpc)] - (char*)bytecodes[this->dtt->at(this->vpc)-1];
        std::string codePart = bytecodes[this->dtt->at(this->vpc)]();
        auto toAddSize = codePart.size();
        if(endCompiledBlock + toAddSize - startCompiledBlock > maxBlockLength)
            break;

//        memcpy(endCompiledBlock, bytecodes[this->dtt->at(this->vpc)], toAddSize);
        memcpy(endCompiledBlock, codePart.c_str(), toAddSize);
        endCompiledBlock += toAddSize;
        this->vpc++;
        instructionsCompiled++;
    }
    int mprotectSuccess = mprotect(startCompiledBlock, endCompiledBlock - startCompiledBlock, PROT_READ | PROT_EXEC);
    if(mprotectSuccess != 0)
        throw VMRuntimeException("Can't mprotect memory, error: " + std::string(strerror(errno)));

    this->jit->insert(std::make_pair(this->vpc, (dtt_func)startCompiledBlock));
    return (dtt_func)startCompiledBlock;
}
 */
/* Function End */

std::string vm_prolog() {
    code_start:
    auto currentFunctionCal = VM::getCurrentFunction;
    auto currentFunction = currentFunctionCal();
    dtt_arg *arg0;
    dtt_arg *arg1;
    dtt_arg *arg2;
    int val0, val1, val2;
    code_end:;

    return std::string(reinterpret_cast<const char*>(vm_prolog), reinterpret_cast<const char*>(&&code_end));
}
std::string vm_epilog() {

}
void vm_schedule() {}

std::string vm_assign(){
    Function* currentFunction;
    dtt_arg *arg0;
    dtt_arg *arg1;
    dtt_arg *arg2;
    int val0, val1, val2;

    auto codeStr = std::string(reinterpret_cast<const char*>(&&code_start), reinterpret_cast<const char*>(&&code_end));
    if(codeStr.size() != 0)
        goto code_end;

    code_start:
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();

    val1 = arg1->valInt;
    if(arg1->type == VAR)
        val1 = currentFunction->var_table[arg1->valStr];

    currentFunction->var_table[arg0->valStr] = val1;
    code_end:;

    return codeStr;
};
std::string vm_print() {
    Function* currentFunction;
    dtt_arg *arg0;
    dtt_arg *arg1;
    dtt_arg *arg2;
    int val0, val1, val2;

    return std::string(reinterpret_cast<const char*>(&&code_start), reinterpret_cast<const char*>(&&code_end));

    code_start:
    arg0 = currentFunction->getNextArg();

    val0 = arg0->valInt;
    if(arg0->type == VAR)
        val0 = currentFunction->var_table[arg0->valStr];

    VM::print(std::to_string(val0));
    code_end:;
}

void vm_call() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    currentFunction->anotherFunctionCalled = true;
    currentFunction->return_variable = arg1->valStr;

    auto newFunction = VM::getNewFunction(arg0->valStr);
    std::vector<int> newFunctionArgs;

    for (int i = 0; i < newFunction->arg_table_size; ++i) {
        auto valArg = currentFunction->getNextArg();
        int valNext = valArg->valInt;
        if(valArg->type == VAR)
            valNext = currentFunction->var_table[valArg->valStr];
        newFunctionArgs.push_back(valNext);
    }

    newFunction->setArguments(newFunctionArgs);
    newFunction->returnFunction = currentFunction;

    VM::getCurrentThread()->currect_function = newFunction;
};
void vm_return(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();

    int val0 = arg0->valInt;
    if(arg0->type == VAR)
        val0 = currentFunction->var_table[arg0->valStr];

    if(currentFunction->returnFunction != nullptr) {
        currentFunction->returnFunction->var_table[currentFunction->returnFunction->return_variable] = val0;
        currentFunction->returnFunction->anotherFunctionCalled = false;
    }
    VM::getCurrentThread()->currect_function = currentFunction->returnFunction;
    delete currentFunction;
};

void vm_send(){};
void vm_recv(){};
void vm_start(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    auto newThread = VM::getNewThread(arg1->valStr, arg0->valStr);

    std::vector<int> newFunctionArgs;
    for (int i = 0; i < newThread->currect_function->arg_table_size; ++i) {
        auto valArg = currentFunction->getNextArg();
        int valNext = valArg->valInt;
        if(valArg->type == VAR)
            valNext = currentFunction->var_table[valArg->valStr];
        newFunctionArgs.push_back(valNext);
    }

    newThread->currect_function->setArguments(newFunctionArgs);
};
void vm_join(){

};
void vm_stop(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();

    VM::stopThread(arg0->valStr);
};

std::string vm_add() {
    Function* currentFunction;
    dtt_arg *arg0;
    dtt_arg *arg1;
    dtt_arg *arg2;
    int val0, val1, val2;

    return std::string(reinterpret_cast<const char*>(&&code_start), reinterpret_cast<const char*>(&&code_end));
    code_start:
    arg0 = currentFunction->getNextArg();
    arg1 = currentFunction->getNextArg();
    arg2 = currentFunction->getNextArg();

    val1 = arg1->valInt;
    if(arg1->type == VAR)
        val1 = currentFunction->var_table[arg1->valStr];

    val2 = arg2->valInt;
    if(arg2->type == VAR)
        val2 = currentFunction->var_table[arg2->valStr];

    currentFunction->var_table[arg0->valStr] = val1 + val2;
    code_end:;
}
void vm_sub() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1->valInt;
    if(arg1->type == VAR)
        val1 = currentFunction->var_table[arg1->valStr];

    int val2 = arg2->valInt;
    if(arg2->type == VAR)
        val2 = currentFunction->var_table[arg2->valStr];

    currentFunction->var_table[arg0->valStr] = val1 - val2;
};
void vm_div() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1->valInt;
    if(arg1->type == VAR)
        val1 = currentFunction->var_table[arg1->valStr];

    int val2 = arg2->valInt;
    if(arg2->type == VAR)
        val2 = currentFunction->var_table[arg2->valStr];

    currentFunction->var_table[arg0->valStr] = val1 / val2;
};
void vm_mul() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1->valInt;
    if(arg1->type == VAR)
        val1 = currentFunction->var_table[arg1->valStr];

    int val2 = arg2->valInt;
    if(arg2->type == VAR)
        val2 = currentFunction->var_table[arg2->valStr];

    currentFunction->var_table[arg0->valStr] = val1 * val2;
};

