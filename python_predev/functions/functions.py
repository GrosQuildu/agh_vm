from copy import deepcopy

from bytecode import bytecodes_table, TOKEN_TYPES, const_n2s
from exceptions import *


class Function(object):
    def __init__(self, FunctionTemplate, *args):
        self.name = FunctionTemplate.name
        self.arg_table = deepcopy(FunctionTemplate.arg_table)
        self.var_table = deepcopy(FunctionTemplate.var_table)
        self.dtt = deepcopy(FunctionTemplate.dtt)
        self.dtt_args = deepcopy(FunctionTemplate.dtt_args)
        self.dtt_removed = []

        self.return_function = None
        self.vpc = 0
        self.return_variable = None

        # resolve function arguments
        if len(args) != len(self.arg_table):
            raise VMRuntimeError("Wrong number of arguments")

        for i, (arg_type, arg_val) in enumerate(self.dtt_args):
            if arg_type == TOKEN_TYPES["ARG"]:
                self.dtt_args[i] = (TOKEN_TYPES["CONST"], args[arg_val])

    def run(self):
        self.dtt[self.vpc]()

    def __str__(self):
        fun_str = self.name
        if self.return_function is not None:
            fun_str += " <- {}".format(self.return_function.name)
        fun_str += "\n"

        fun_str += " VPC:      {}\n".format(self.vpc)

        
        how_much = 15
        where_now = self.vpc - max(0, self.vpc - how_much)
        fun_str += " DTT:\n"
        for i, function in enumerate(self.dtt[max(0, self.vpc - how_much):self.vpc+how_much]):
            fun_str += "   " + function.__name__
            if i == where_now:
                fun_str += "  <"
            fun_str += "\n"
        fun_str += "\n"

        fun_str += " DTT args: \n {}\n\n".format('\n '.join([const_n2s(arg[0]) +' - '+ str(arg[1]) for arg in self.dtt_args[:20]]))
        fun_str += " VAR:     \n {}\n".format('\n '.join([var_name +' = '+ str(var_value) for var_name, var_value in self.var_table.items()]))
        return fun_str


class FunctionTemplate(object):
    def __init__(self, code):
        self.name, self.arg_table, self.var_table, self.dtt, self.dtt_args = self.parse_code(code)

    def __call__(self, *args):
        return Function(self, *args)
        

    def parse_code(self, code):
        dtt = []
        dtt_args = []
        tokens = code.split('\n')
        tokens = list(filter(None, tokens))

        bytecode_function_def = tokens[0]
        bytecode_function_def = bytecode_function_def.split(' ')
        bytecode_function_end = tokens[-1]
        tokens = tokens[1:-1]

        if bytecode_function_def[0] != "DEF":
            raise ParserError("Wrong function definition")
        if len(bytecode_function_def) - 1 != len(bytecodes_table["DEF"][1]):
            raise ParserError("Wrong number of arguments in function definition '{}'".format(' '.join(bytecode_function_def)))
        if bytecode_function_end != "END":
            raise ParserError("Wrong function ending")

        function_name = bytecode_function_def[1]
        function_arg_count = int(bytecode_function_def[2])
        arg_table = [0] * function_arg_count
        var_table = {}

        bytecode_counter = 0
        var_declaration_end = False
        previous_loads = 0

        while bytecode_counter < len(tokens):
            bytecode = tokens[bytecode_counter].split(' ')
            bytecode_name = bytecode[0]

            if bytecode_name not in bytecodes_table.keys():
                raise ParserError("Bytecode {} not known".format(bytecode_name))

            if bytecode_name in ["DECLARE", "LOAD"]:
                if len(bytecode) - 1 != len(bytecodes_table[bytecode_name][1]):
                    raise ParserError("Wrong number of arguments to bytecode '{}'".format(''.join(bytecode)))

            if bytecode_name == "DECLARE":
                if var_declaration_end == False:
                    var_table[bytecode[1]] = 0
                else:
                    raise ParserError("Declaration after non-declaration")
            elif bytecode_name == "RETURN":
                dtt.append(bytecodes_table[bytecode_name][0])
                break
            else:
                var_declaration_end = True

                if bytecode_name.startswith("LOAD"):
                    previous_loads += 1

                if bytecode_name == "LOAD":
                    if bytecode[1].startswith('ARG_'):
                        dtt_args.append((TOKEN_TYPES["ARG"], int(bytecode[1][4:])))
                    else:
                        dtt_args.append((TOKEN_TYPES["CONST"], int(bytecode[1])))
                elif bytecode_name == "LOADV":
                    if bytecode[1] in var_table.keys():
                        dtt_args.append((TOKEN_TYPES["VAR"], bytecode[1]))
                    else:
                        raise ParserError("Variable {} not known".format(bytecode[1]))
                elif bytecode_name == "LOADF":
                    dtt_args.append((TOKEN_TYPES["FUNC"], bytecode[1]))
                elif bytecode_name == "LOADT":
                    dtt_args.append((TOKEN_TYPES["THREAD"], bytecode[1]))
                else:
                    if bytecode_name not in ("CALL", "START") and len(bytecodes_table[bytecode_name][1]) != previous_loads:
                        raise ParserError("Wrong number of loads before bytecode '{}'".format(''.join(bytecode)))
                    # todo test if corrent args
                    dtt.append(bytecodes_table[bytecode_name][0])
                    previous_loads = 0

            bytecode_counter += 1

        return function_name, arg_table, var_table, dtt, dtt_args
