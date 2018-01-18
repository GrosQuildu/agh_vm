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


dtt_arg FunctionFactory::parse_load(std::string line, int argTableSize, std::map<std::string, int> &varTable) {
    std::string bytecode;
    std::string bytecodeArg;
    dtt_arg dttArgTmp = dtt_arg();

    if(line.find(" ") == line.npos)
        throw ParserException("Argument to LOAD bytecode required");
    bytecode = line.substr(0, line.find(" "));
    bytecodeArg = line.substr(line.find(" ")+1);

    if(bytecode == "LOAD") {
        if(startswith(bytecodeArg, "ARG_")) {
            dttArgTmp.type = ARG;
            try {
                dttArgTmp.valInt = std::stoi(bytecodeArg.substr(4));
            } catch(std::invalid_argument) {
                throw ParserException("Invalid integer after ARG_: " + bytecodeArg);
            }
            if(dttArgTmp.valInt >= argTableSize)
                throw ParserException("Integer after ARG_ too large");
            if(dttArgTmp.valInt < 0)
                throw ParserException("Integer after ARG_ too small");
        } else {
            dttArgTmp.type = CONST;
            try {
                dttArgTmp.valInt = std::stoi(bytecodeArg);
            } catch(std::invalid_argument) {
                throw ParserException("Invalid const: " + bytecodeArg);
            }
        }
    } else if(bytecode == "LOADV") {
        if(varTable.find(bytecodeArg) == varTable.end())
            throw ParserException("Variable " + bytecodeArg + " not found");
        dttArgTmp.type = VAR;
        dttArgTmp.valStr = bytecodeArg;
    } else if(bytecode == "LOADF") {
        dttArgTmp.type = FUNC;
        dttArgTmp.valStr = bytecodeArg;
    } else if(bytecode == "LOADT") {
        dttArgTmp.type = THREAD;
        dttArgTmp.valStr = bytecodeArg;
    } else {
        throw ParserException("Wrong LOAD bytecode: " + bytecode);
    }
    return dttArgTmp;
}

std::pair<bool, int> FunctionFactory::check_instruction(std::string line, std::vector<dtt_arg> &dttArgsVector, int argCounter) {
    std::vector<std::set<unsigned char>> &requiredArgs = bytecodeMapping.at(line).second;
    auto argToCheck = dttArgsVector.end() - argCounter;
    bool unknownNumberOfArguments = false;
    int argumentsChecked = 0;

    for(auto requiredArgTuple = requiredArgs.begin(); requiredArgTuple != requiredArgs.end(); ++requiredArgTuple) {
        // empty set -> unknown (at parsing time) number of elements
        if(requiredArgTuple->empty()) {
            unknownNumberOfArguments = true;
            break;
        }
        if(requiredArgTuple->find((*argToCheck).type) == requiredArgTuple->end()) {
            std::string errMsg = "Wrong arguments (calls to LOADs) for " + line + "\n";
            errMsg += "Required: " + vector2string(requiredArgs) + "\n";
            errMsg += "Found: [";
            for (auto it = dttArgsVector.end() - argCounter; it != dttArgsVector.end(); ++it)
                errMsg += argTypeToStr((*it).type) + ", ";
            errMsg += "]";
            throw ParserException(errMsg);
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
    int argTableSize = 0;

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
            argTableSize = std::stoi(line);
        } catch(std::invalid_argument) {
            throw ParserException("Incorrect ARGS_COUNT in DEF");
        }
        if(argTableSize < 0)
            throw ParserException("Negative ARGS_COUNT in DEF");
    }

    // DEFINE -  declarations of variables
    std::map<std::string, int> varTable;
    std::string varName;

    while (std::getline(codeFile, line)) {
        trim(line);
        if(line.empty() || startswith(line, "//") || startswith(line, "#"))
            continue;
        if(startswith(line, "DECLARE ")) {
            varName = line.substr(8);
            if(line.empty())
                throw ParserException("VAR_NAME after DEFINE required");
            varTable[varName] = 0;
        } else
            break;
    }

    // bytecodes
    std::vector<dtt_func> *dtt = new std::vector<dtt_func>;
    std::vector<dtt_arg> dttArgsVector;
    std::string bytecode;

    std::forward_list<std::tuple<std::string, int, int>> calledFunctionsToCheck;
    int argCounter = 0;

    do {
        trim(line);
        if(line.empty() || startswith(line, "//") || startswith(line, "#"))
            continue;
        // PARSE LOADS
        if(startswith(line, "LOAD")) {
            dtt_arg dttArgTmp = parse_load(line, argTableSize, varTable);
            dttArgsVector.push_back(dttArgTmp);
            argCounter++;
        // PARSE END
        } else if(line == "END") {
            endReached = true;
        // PARSE DEFINE
        } else if(startswith(line, "DEFINE")) {
            throw ParserException("Variables definitions can appear only at the beginning og the function");
        // PARSE OTHERS
        } else if(bytecodeMapping.find(line) != bytecodeMapping.end()) {
            auto instructionChecked = check_instruction(line, dttArgsVector, argCounter);
            bool unknownNumberOfArguments = instructionChecked.first;
            int argumentsChecked = instructionChecked.second;
            if(unknownNumberOfArguments) {
                std::string functionToCheckName = dttArgsVector.at(dttArgsVector.size() - argCounter).valStr;
                calledFunctionsToCheck.push_front(std::make_tuple(functionToCheckName,
                                                                  dttArgsVector.size() - argCounter,
                                                                  argCounter - argumentsChecked));
            }
            argCounter = 0;
            dtt->push_back(bytecodeMapping.at(line).first);
        } else {
            throw ParserException("Unknown bytecode: " + line);
        }
    } while (!endReached && std::getline(codeFile, line));
    codeFile.close();

    if(!endReached)
        throw ParserException("END not fund");

    std::forward_list<dtt_arg>* dttArgs = new std::forward_list<dtt_arg>(dttArgsVector.begin(), dttArgsVector.end());

    return std::make_pair(new FunctionPrototype(name, dtt, dttArgs, argTableSize, varTable), calledFunctionsToCheck);
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
            std::string errorMsg = "Function " + functionToCheckName + " called with wrong number of arguments";
            errorMsg += "\nHave: " + std::to_string(functionArgumentsCount);
            errorMsg += "\nShould be: " + std::to_string(fp->argTableSize);
            throw ParserException(errorMsg);
        }
    }
}

Function* FunctionFactory::makeFunction(std::string functionName) {
    auto functionPrototype = this->functionsPrototypes[functionName];
    if(functionPrototype == nullptr)
        throw VMRuntimeException("Can't find function with name " + functionName + " in FunctionFactory");
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
                                     std::forward_list<dtt_arg>* dttArgs, int argTableSize,
                                     std::map<std::string,int>varTable) {
    this->name = name;
    this->dtt = dtt;
    this->dttArgs = dttArgs;
    this->argTableSize = argTableSize;
    this->varTable = varTable;
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
    this->dttArgs = functionPrototype.dttArgs;
    this->dttArgsIt = this->dttArgs->begin();

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
}

jit_func Function::compile() {
    auto vm = VM::getVM();
    std::string blocksDir = vm.blocksDir;
    std::string blockName = this->name + "_block_" + std::to_string(this->vpc);

    // compile only if rebuild option is true or it's not already compiled
    struct stat buffer;
    if(vm.rebuild || stat((blocksDir + blockName + ".so").c_str(), &buffer) != 0) {

        // prepare blocks as string with cpp code to compile
        std::array<dtt_func, 3> blockingBytecodes = {vm_join, vm_recv};  // must be at the beginning of the block
        std::array<dtt_func, 1> changingContextBytecodes = {vm_call};  // must be at the end of the block

        unsigned long blockCounter = 0;
        std::string blocks = vm_prolog(blockName);
        bool nextIsBlockingBytecode = false;
        bool nextIsChangingContextBytecode = false;

        while((*this->maxBlockSizePtr == 0 || blockCounter < *this->maxBlockSizePtr) &&
                this->vpc + blockCounter < this->dtt->size() ) {

            nextIsBlockingBytecode = std::find(blockingBytecodes.begin(), blockingBytecodes.end(),
                                               dtt->at(this->vpc + blockCounter)) != blockingBytecodes.end();
            if(nextIsBlockingBytecode && blockCounter != 0)
                break;

            nextIsChangingContextBytecode = std::find(changingContextBytecodes.begin(), changingContextBytecodes.end(),
                                               dtt->at(this->vpc + blockCounter)) != changingContextBytecodes.end();
            if(nextIsChangingContextBytecode)
                break;

            blocks += dtt->at(this->vpc + blockCounter)();
            blockCounter++;
        }

        if(nextIsChangingContextBytecode)
            blocks += dtt->at(this->vpc + blockCounter)();

        blocks += vm_epilog();

        // prepare directory
        if(mkdir(blocksDir.c_str(), S_IRWXU) != 0 && errno != EEXIST)
            throw VMRuntimeException(strerror(errno));

        // save blocks to file
        std::ofstream(blocksDir + blockName + ".cpp") << blocks;

        // run external compiler
        std::string compilerCommand = std::string(CXX_COMPILER) + " -g -c -fPIC -o " + blocksDir + blockName + ".o  " + blocksDir + blockName + ".cpp";
        vm.print("Compiling with: " + compilerCommand);
        if(std::system(compilerCommand.c_str()) != 0)
            throw VMRuntimeException("Compiler error");

        compilerCommand = std::string(CXX_COMPILER) + " -g -shared -o " + blocksDir + blockName + ".so  " + blocksDir + blockName + ".o";
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

dtt_arg Function::getNextArg(bool increment) {
    dtt_arg nextArg = *this->dttArgsIt;
    if(increment)
        this->dttArgsIt++;
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

