#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
~PP
'''

class ParserError(Exception):
    pass


def vm_DECLARE():
    pass

def vm_ASSIGN():
    pass

def vm_LOAD():
    pass

def vm_DEF():
    pass

def vm_END():
    pass

def vm_CALL():
    pass

def vm_RETURN():
    pass

def vm_SEND():
    pass

def vm_RECV():
    pass

def vm_START():
    pass

def vm_JOIN():
    pass

def vm_STOP():
    pass

def vm_ADD():
    pass

def vm_SUB():
    pass

def vm_DIV():
    pass

def vm_MUL():
    pass


class Function(object):
    bytecodes = {
        "DECLARE": (vm_DECLARE, 1),
        "ASSIGN": (vm_ASSIGN, 2),
        "LOAD": (vm_LOAD, 1),
        "DEF": (vm_DEF, 1),
        "END": (vm_END, 1),
        "CALL": (vm_CALL, 1),
        "RETURN": (vm_RETURN, 1),
        "SEND": (vm_SEND, 1),
        "RECV": (vm_RECV, 1),
        "START": (vm_START, 1),
        "JOIN": (vm_JOIN, 1),
        "STOP": (vm_STOP, 1),
        "ADD": (vm_ADD, 1),
        "SUB": (vm_SUB, 1),
        "DIV": (vm_DIV, 1),
        "MUL":(vm_MUL,, 1),
    }

    def __init__(self, code):
        self.name, self.arg_table, self.var_table, self.dtt, self.dtt_args = self.parse_code(code)
        self.return_fit = None
        self.vpc = 0
        

    def parse_code(self, code):
        dtt = []
        dtt_args = []
        bytecodes = code.split('\n')

        bytecode_function_def = bytecodes[0]
        bytecode_function_def = bytecode_function_def.split(' ')
        bytecode_function_end = bytecodes[-1]
        bytecodes = bytecodes[1:-1]

        if bytecode_function_def[0] != "DEF":
            raise ParserError("Wrong function definition")
        if len(bytecode_function_def) -1 != Function.bytecodes["DEF"]:
            raise ParserError("Wrong number of arguments in function definition '{}'".format(bytecode_function_def))
        if bytecode_function_end != "END":
            raise ParserError("Wrong function ending")

        function_name = bytecode_function_def[1]
        function_arg_count = int(bytecode_function_def[2])
        arg_table = [None] * function_arg_count
        var_table = {}

        bytecode_counter = 0
        var_declaration_end = False

        while bytecode_counter < len(bytecodes):
            bytecode = bytecodes[bytecode_counter].split(' ')
            bytecode_name = bytecode[0]

            if bytecode_name not in Function.bytecodes.keys():
                raise ParserError("Bytecode {} not known".format(bytecode_name))

            if len(bytecode) - 1 != Function.bytecodes[bytecode_name][1]:
                raise ParserError("Wrong number of arguments to bytecode '{}'".format(bytecode))

            if var_declaration_end == False:
                if bytecode_name == "DECLARE":
                    var_table[bytecode[1]] = None
                else:
                    var_declaration_end = True
            elif bytecode_name == "DECLARE":
                raise ParserError("Declaration after non-declaration")

            if bytecode_name == "LOAD":
                if bytecode[1] in var_table.keys():
                    dtt_args.append((0, bytecode[1]))
                elif bytecode[1].startswith('ARG_'):
                    dtt_args.append((1, bytecode[1][4:]))
                else:
                    dtt_args.append((2, bytecode[1]))


            dtt.append(Function.bytecodes[bytecode_name][0])
            for bytecode_arg in bytecode[1:]:
                dtt_args.append(bytecode_arg)




class Thread(object):
    def __init__(self, function):
        self.function = function
        self.recv_table = []
        self.status = None


class ThreadManager(object):
    def __init__(self):
        self.threads = []
        self.current_thread = -1


if __name__ == "__main__":
    thread_manager = ThreadManager()