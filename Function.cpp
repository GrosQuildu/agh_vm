//
// Created by gros on 13.12.17.
//

#include "Function.h"


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



void FunctionFactory::addFunction(std::string codePath) {
    auto functionPrototype = parseCode(codePath);
    FunctionFactory::functionsPrototypes[functionPrototype->name] = functionPrototype;
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

dtt_func FunctionFactory::parse_instruction(std::string line, std::vector<dtt_arg> &dtt_args_vector) {
    std::vector<std::set<char>> &required_args = bytecodeMapping.at(line).second;
    int counter = (int)dtt_args_vector.size() - 1;

    for(auto required_arg_tuple = required_args.rbegin(); required_arg_tuple != required_args.rend(); ++required_arg_tuple) {
        // empty set -> unknown (at paring time) number of elements
        if(required_arg_tuple->empty())
            break;
        if(required_arg_tuple->find(dtt_args_vector.at(counter--).type) == required_arg_tuple->end()) {
            std::string err_msg = "Wrong arguments (calls to LOADs) for " + line + "\n";
            err_msg += "Required: " + vector2string(required_args) + "\n";
            err_msg += "Found: [";
            for (int i = 0; i < required_args.size(); ++i) {
                if(i + 1 <= dtt_args_vector.size())
                    err_msg += const2str(dtt_args_vector.at(dtt_args_vector.size() - i - 1).type) + ", ";
                else
                    break;
            }
            err_msg += "]";
            throw ParserException(err_msg);
        }
    }
    return bytecodeMapping.at(line).first;
}

FunctionPrototype* FunctionFactory::parseCode(std::string codePath) {
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

    do {
        trim(line);
        if(!line.empty() && !startswith(line, "//")) {
            if(startswith(line, "LOAD")) {
                dtt_arg dtt_arg_tmp = parse_load(line, arg_table_size, var_table);
                dtt_args_vector.push_back(dtt_arg_tmp);
            } else if(line == "END") {
                endReached = true;
            } else if(startswith(line, "DEFINE")) {
                throw ParserException("Variables definitions can appear only at the beginnig og the function");
            } else if(bytecodeMapping.find(line) != bytecodeMapping.end()) {
                dtt_func dtt_tmp = parse_instruction(line, dtt_args_vector);
                dtt_vector.push_back(dtt_tmp);
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

    return new FunctionPrototype(name, dtt, dtt_args, arg_table_size, var_table);
}

void FunctionFactory::initialize(std::string codeDirPath) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (codeDirPath.c_str())) == NULL) {
        throw ParserException("Path not found: " + codeDirPath);

    }

    while ((ent = readdir (dir)) != NULL) {
        std::string filename = ent->d_name;
        if(endswith(filename, bytecodeExtension)) {
            this->addFunction(codeDirPath + filename);
        }
    }
    closedir (dir);
}

Function* FunctionFactory::makeFunction(std::string functionName) {
    auto functionPrototype = FunctionFactory::functionsPrototypes[functionName];
    return functionPrototype->generate();
}

bool FunctionFactory::haveFunction(std::string functionPrototypeName) {
    return this->functionsPrototypes.find(functionPrototypeName) != this->functionsPrototypes.end();
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

Function* FunctionPrototype::generate() {
    return new Function(*this);
}



Function::Function(FunctionPrototype& functionPrototype) {
    this->name = functionPrototype.name;
    this->dtt = functionPrototype.dtt;

    this->dtt_args = functionPrototype.dtt_args;

    this->arg_table = new int[functionPrototype.arg_table_size];
    this->var_table = functionPrototype.var_table;

    this->vpc = this->dtt->begin();
}

void Function::run() {
    std::cout << *this << std::endl;
    while(this->vpc != this->dtt->end())
        (*this->vpc++)(*this);
}

dtt_arg& Function::getNextArg() {
    dtt_arg& nextArg = this->dtt_args->front();
    this->dtt_args->pop_front();
    return nextArg;
}

std::ostream& operator<<(std::ostream& s, const Function& function) {
    s << function.name << "\n";
    s << "CODE:\n";
    for (auto bytecode = function.dtt->begin(); bytecode != function.dtt->end(); bytecode++) {
        for (auto it=bytecodeMapping.begin(); it!=bytecodeMapping.end(); ++it)
            if(it->second.first == *bytecode) {
                s << "  " << it->first << "\n";
                break;
            }
    }
    return s;
}


void vm_assign(Function &currentFunction){};
void vm_print(Function &currentFunction) {
    auto arg0 = currentFunction.getNextArg();

    int val0 = arg0.valInt;
    if(arg0.type == VAR)
        val0 = currentFunction.var_table[arg0.valStr];

    std::cout << val0 << "\n";
}

void vm_call(Function &currentFunction){};
void vm_return(Function &currentFunction){};

void vm_send(Function &currentFunction){};
void vm_recv(Function &currentFunction){};
void vm_start(Function &currentFunction){};
void vm_join(Function &currentFunction){};
void vm_stop(Function &currentFunction){};

void vm_add(Function &currentFunction) {
    auto arg0 = currentFunction.getNextArg();
    auto arg1 = currentFunction.getNextArg();
    auto arg2 = currentFunction.getNextArg();

    int val1 = arg1.valInt;
    if(arg1.type == VAR)
        val1 = currentFunction.var_table[arg1.valStr];

    int val2 = arg2.valInt;
    if(arg2.type == VAR)
        val2 = currentFunction.var_table[arg2.valStr];

    currentFunction.var_table[arg0.valStr] = val1 + val2;
}
void vm_sub(Function &currentFunction){};
void vm_div(Function &currentFunction){};
void vm_mul(Function &currentFunction){};

