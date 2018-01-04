//
// Created by gros on 13.12.17.
//

#include "Function.h"
#include "VM.h"

const std::string const2str(char c) {
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
    std::vector<std::set<char>> &required_args = bytecodeMapping.at(line).second;
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
    std::vector<dtt_func> dtt_vector;
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
                dtt_vector.push_back(bytecodeMapping.at(line).first);
            } else {
                throw ParserException("Unknown bytecode: " + line);
            }
        }
    } while (!endReached && std::getline(codeFile, line));
    codeFile.close();

    if(!endReached)
        throw ParserException("END not fund");

    std::forward_list<dtt_func>* dtt = new std::forward_list<dtt_func>(dtt_vector.begin(), dtt_vector.end());
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
        int functionArgumentsStartPosition = std::get<1>(*it);
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

void FunctionFactory::setSchedulingFrequency(int frequency) {
    for(auto it = this->functionsPrototypes.begin(); it != this->functionsPrototypes.end(); it++) {
        if(frequency == 0)
            (*it).second->clearSchedulingBytecodes();
        else
            (*it).second->setSchedulingBytecodes(frequency);
    }
}


FunctionPrototype::FunctionPrototype(std::string name, std::forward_list<dtt_func>* dtt,
                                     std::forward_list<dtt_arg>* dtt_args, int arg_table_size,
                                     std::map<std::string,int>var_table) {
    this->name = name;
    this->dtt = dtt;
    this->dtt_args = dtt_args;
    this->arg_table_size = arg_table_size;
    this->var_table = var_table;
}

FunctionPrototype::~FunctionPrototype() {
    delete this->dtt;
    delete this->dtt_args;
    this->dtt = nullptr;
    this->dtt_args = nullptr;
}

Function* FunctionPrototype::generate() {
    return new Function(*this);
}

void FunctionPrototype::setSchedulingBytecodes(int frequency) {
    dtt_func blockingBytecodes[] {vm_schedule, vm_call, vm_return, vm_recv, vm_join};
    auto it = this->dtt->begin();
    while(it != this->dtt->end()) {
        for (int i = 0; i < frequency && it != this->dtt->end(); i++, it++);
        // do not set vm_schedule after bytecode that may loose control
        if(it != this->dtt->end() && std::find(std::begin(blockingBytecodes), std::end(blockingBytecodes), *it) != std::end(blockingBytecodes))
            this->dtt->insert_after(it, &vm_schedule);
    }
}

void FunctionPrototype::clearSchedulingBytecodes() {
    this->dtt->remove(vm_schedule);
}


Function::Function(FunctionPrototype& functionPrototype) {
    this->name = functionPrototype.name;
    this->dtt = functionPrototype.dtt;
    this->dtt_args = new std::forward_list<dtt_arg>(functionPrototype.dtt_args->begin(), functionPrototype.dtt_args->end());

    this->arg_table_size = functionPrototype.arg_table_size;
    this->var_table = functionPrototype.var_table;

    this->vpc = this->dtt->begin();

    this->anotherFunctionCalled = false;
    this->returnFunction = nullptr;
    this->return_variable = "";
}

Function::~Function() {
    delete this->dtt_args;
    this->dtt_args = nullptr;
}

void Function::run() {
    std::cout << *this << std::endl;
    while(this->vpc != this->dtt->end() && !this->anotherFunctionCalled)
        (*this->vpc++)();
}

dtt_arg& Function::getNextArg() {
    dtt_arg& nextArg = this->dtt_args->front();
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


void vm_schedule() {

}

void vm_assign(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->var_table[arg1.valStr];

    currentFunction->var_table[arg0.valStr] = val1;
};
void vm_print() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();

    int val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction->var_table[arg0.valStr];

    std::cout << val0 << "\n";
}

void vm_call(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();

    currentFunction->anotherFunctionCalled = true;
    currentFunction->return_variable = arg1.valStr;

    auto newFunction = VM::getNewFunction(arg0.valStr);
    std::vector<int> newFunctionArgs;

    for (int i = 0; i < newFunction->arg_table_size; ++i) {
        auto valArg = currentFunction->getNextArg();
        int valNext = valArg.valInt;
        if(valArg.type == VAR)
            valNext = currentFunction->var_table[valArg.valStr];
        newFunctionArgs.push_back(valNext);
    }

    newFunction->setArguments(newFunctionArgs);
    newFunction->returnFunction = currentFunction;

    VM::getCurrentThread()->currect_function = newFunction;
};
void vm_return(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();

    int val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction->var_table[arg0.valStr];

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

    auto newThread = VM::getNewThread(arg1.valStr, arg0.valStr);

    std::vector<int> newFunctionArgs;
    for (int i = 0; i < newThread->currect_function->arg_table_size; ++i) {
        auto valArg = currentFunction->getNextArg();
        int valNext = valArg.valInt;
        if(valArg.type == VAR)
            valNext = currentFunction->var_table[valArg.valStr];
        newFunctionArgs.push_back(valNext);
    }

    newThread->currect_function->setArguments(newFunctionArgs);
};
void vm_join(){

};
void vm_stop(){
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();

    VM::stopThread(arg0.valStr);
};

void vm_add() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->var_table[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->var_table[arg2.valStr];

    currentFunction->var_table[arg0.valStr] = val1 + val2;
}
void vm_sub() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->var_table[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->var_table[arg2.valStr];

    currentFunction->var_table[arg0.valStr] = val1 - val2;
};
void vm_div() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->var_table[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->var_table[arg2.valStr];

    currentFunction->var_table[arg0.valStr] = val1 / val2;
};
void vm_mul() {
    auto currentFunction = VM::getCurrentFunction();
    auto arg0 = currentFunction->getNextArg();
    auto arg1 = currentFunction->getNextArg();
    auto arg2 = currentFunction->getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction->var_table[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction->var_table[arg2.valStr];

    currentFunction->var_table[arg0.valStr] = val1 * val2;
};

