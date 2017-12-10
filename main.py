#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
~PP
'''

from glob import glob
import string
from copy import deepcopy
import curses
from time import sleep
from random import randint


class ParserError(Exception):
    pass

class VMRuntimeError(Exception):
    pass

program = None
def get_context():
    ct = program.thread_manager.current_thread
    ct.refresh()
    sleep(1)
    return ct

VAR = 0
ARG = 1
CONST = 2
FUNC = 3
THREAD = 4
def const_n2s(number):
    c = ['VAR', 'ARG', 'CONST', 'FUNC', 'THREAD']
    if number >=0 and number < len(c):
        return c[number]
    else:
        return ''


# def load_arg(arg):
#     arg_type = arg[0]
#     arg_val = arg[1]
#     if arg_type == VAR:
#         if arg_val not in thread_manager.current_function.var_table.keys():
#             raise ParserError("Variable {} not declared".format(arg_val))
#         return 


def vm_DECLARE():
    pass

def vm_LOAD():
    pass

def vm_SCHEDULE():
    cf = get_context().current_function
    cf.vpc += 1

    program.thread_manager.schedule()

def vm_ASSIGN():
    cf = get_context().current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    cf.dtt_args = cf.dtt_args[2:]

    if arg1_type == VAR:
        arg1_val = cf.var_table[arg1_val]
    
    cf.var_table[arg0_val] = arg1_val

    cf.vpc += 1
    cf.dtt[cf.vpc]()

def vm_DEF():
    pass

def vm_END():
    pass

def vm_CALL():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    cf.dtt_args = cf.dtt_args[2:]

    cf.vpc += 1

    # prepare new function
    function_to_call_template = list(filter(lambda x: x.name == arg0_val, program.functions))
    if len(function_to_call_template) < 1:
        raise ParserError("Function {} not known".format(arg0_val))
    else:
        function_to_call_template = function_to_call_template[0]

    args_count = len(function_to_call_template.arg_table)
    args = [cf.dtt_args[i] for i in range(args_count)]
    args = map(lambda arg: cf.var_table[arg[1]] if arg[0] == VAR else arg[1], args)
    cf.dtt_args = cf.dtt_args[args_count:]
    function_to_call = function_to_call_template(*args)

    # set return values
    function_to_call.return_function = cf
    ct.current_function = function_to_call
    cf.return_variable = arg1_val
    function_to_call.run()


def vm_RETURN():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    cf.dtt_args = cf.dtt_args[1:]

    if arg0_type == VAR:
        arg0_val = cf.var_table[arg0_val]

    if cf.return_function is None:
        program.thread_manager.remove()
    else:
        ct.current_function = cf.return_function
        rv = cf.return_function.return_variable
        cf.return_function.var_table[rv] = arg0_val
        cf.return_function.return_variable = None
        cf.return_function.run()

def vm_SEND():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    cf.dtt_args = cf.dtt_args[2:]

    if arg1_type == VAR:
        arg1_val = cf.var_table[arg1_val]

    thread_to_send = list(filter(lambda x: x.name == arg0_val, program.thread_manager.threads))
    if len(thread_to_send) == 1:
        thread_to_send[0].recv_table.append((ct.name, arg1_val))

    cf.vpc += 1
    cf.dtt[cf.vpc]()

def vm_RECV():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    
    if len(ct.recv_table) == 0:
        program.thread_manager.next()
    else:
        cf.var_table[arg0_val] = ct.recv_table[0][1]
        ct.recv_table = ct.recv_table[1:]

        cf.dtt_args = cf.dtt_args[1:]
        cf.vpc += 1
        cf.dtt[cf.vpc]()

def vm_START():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    cf.dtt_args = cf.dtt_args[2:]

    # prepare new function
    function_to_call_template = list(filter(lambda x: x.name == arg0_val, program.functions))
    if len(function_to_call_template) < 1:
        raise ParserError("Function {} not known".format(arg0_val))
    else:
        function_to_call_template = function_to_call_template[0]

    args_count = len(function_to_call_template.arg_table)
    args = [cf.dtt_args[i] for i in range(args_count)]
    args = map(lambda arg: cf.var_table[arg[1]] if arg[0] == VAR else arg[1], args)
    cf.dtt_args = cf.dtt_args[args_count:]
    function_to_call = function_to_call_template(*args)

    program.thread_manager.add(function_to_call, arg1_val)

    cf.vpc += 1
    cf.dtt[cf.vpc]()


def vm_JOIN():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]

    thread_to_join = list(filter(lambda x: x.name == arg0_val, program.thread_manager.threads))
    if len(thread_to_join) == 0:
        cf.dtt_args = cf.dtt_args[1:]
        cf.vpc += 1
        cf.dtt[cf.vpc]()
    else:
        program.thread_manager.next()

def vm_STOP():
    pass

def vm_ADD():
    cf = get_context().current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    arg2_type, arg2_val = cf.dtt_args[2]
    cf.dtt_args = cf.dtt_args[3:]

    if arg1_type == VAR:
        arg1_val = cf.var_table[arg1_val]
    if arg2_type == VAR:
        arg2_val = cf.var_table[arg2_val]

    cf.var_table[arg0_val] = arg1_val + arg2_val

    cf.vpc += 1
    cf.dtt[cf.vpc]()

def vm_SUB():
    pass

def vm_DIV():
    pass

def vm_MUL():
    pass

def vm_PRINT():
    cf = get_context().current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    cf.dtt_args = cf.dtt_args[1:]

    # print(cf.var_table[arg0_val])
    program.print_terminal(cf.var_table[arg0_val])

    cf.vpc += 1
    cf.dtt[cf.vpc]()


class Function(object):
    def __init__(self, FunctionTemplate, *args):
        self.name = FunctionTemplate.name
        self.arg_table = deepcopy(FunctionTemplate.arg_table)
        self.var_table = deepcopy(FunctionTemplate.var_table)
        self.dtt = deepcopy(FunctionTemplate.dtt)
        self.dtt_args = deepcopy(FunctionTemplate.dtt_args)

        self.return_function = None
        self.vpc = 0
        self.return_variable = None

        # resolve function arguments
        if len(args) != len(self.arg_table):
            raise VMRuntimeError("Wrong number of arguments")

        for i, (arg_type, arg_val) in enumerate(self.dtt_args):
            if arg_type == ARG:
                self.dtt_args[i] = (CONST, args[arg_val])

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
        fun_str += " VARS:     \n {}\n".format('\n '.join([var_name +' = '+ str(var_value) for var_name, var_value in self.var_table.items()]))
        return fun_str


class FunctionTemplate(object):
    bytecodes = {
        "DECLARE": (vm_DECLARE, [VAR]),
        "LOAD": (vm_LOAD, [(CONST, ARG)]),
        "LOADV": (vm_LOAD, [VAR]),
        "LOADF": (vm_LOAD, [FUNC]),
        "LOADT": (vm_LOAD, [THREAD]),
        "ASSIGN": (vm_ASSIGN, [VAR, (VAR, CONST, ARG)]),
        "DEF": (vm_DEF, [FUNC, CONST]),
        "END": (vm_END, []),
        "CALL": (vm_CALL, [FUNC, VAR, (VAR, CONST, ARG)]),
        "RETURN": (vm_RETURN, [(VAR, CONST, ARG)]),
        "SEND": (vm_SEND, [THREAD, (VAR, CONST, ARG)]),
        "RECV": (vm_RECV, [VAR]),
        "START": (vm_START, [FUNC, THREAD]),
        "JOIN": (vm_JOIN, [THREAD]),
        "STOP": (vm_STOP, [THREAD]),
        "ADD": (vm_ADD, [VAR, (VAR, CONST, ARG), (VAR, CONST, ARG)]),
        "SUB": (vm_SUB, [VAR, (VAR, CONST, ARG), (VAR, CONST, ARG)]),
        "DIV": (vm_DIV, [VAR, (VAR, CONST, ARG), (VAR, CONST, ARG)]),
        "MUL": (vm_MUL, [VAR, (VAR, CONST, ARG), (VAR, CONST, ARG)]),
        "PRINT": (vm_PRINT, [(VAR, CONST, ARG)])
    }

    def __init__(self, code):
        self.name, self.arg_table, self.var_table, self.dtt, self.dtt_args = self.parse_code(code)

    def __call__(self, *args):
        return Function(self, *args)
        

    def parse_code(self, code):
        dtt = []
        dtt_args = []
        bytecodes = code.split('\n')
        bytecodes = list(filter(None, bytecodes))

        bytecode_function_def = bytecodes[0]
        bytecode_function_def = bytecode_function_def.split(' ')
        bytecode_function_end = bytecodes[-1]
        bytecodes = bytecodes[1:-1]

        if bytecode_function_def[0] != "DEF":
            raise ParserError("Wrong function definition")
        if len(bytecode_function_def) - 1 != len(FunctionTemplate.bytecodes["DEF"][1]):
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

        while bytecode_counter < len(bytecodes):
            bytecode = bytecodes[bytecode_counter].split(' ')
            bytecode_name = bytecode[0]

            if bytecode_name not in FunctionTemplate.bytecodes.keys():
                raise ParserError("Bytecode {} not known".format(bytecode_name))

            if bytecode_name in ["DECLARE", "LOAD"]:
                if len(bytecode) - 1 != len(FunctionTemplate.bytecodes[bytecode_name][1]):
                    raise ParserError("Wrong number of arguments to bytecode '{}'".format(''.join(bytecode)))

            if bytecode_name == "DECLARE":
                if var_declaration_end == False:
                    var_table[bytecode[1]] = 0
                else:
                    raise ParserError("Declaration after non-declaration")
            elif bytecode_name == "RETURN":
                dtt.append(FunctionTemplate.bytecodes[bytecode_name][0])
                break
            else:
                var_declaration_end = True

                if bytecode_name.startswith("LOAD"):
                    previous_loads += 1

                if bytecode_name == "LOAD":
                    if bytecode[1].startswith('ARG_'):
                        dtt_args.append((ARG, int(bytecode[1][4:])))
                    else:
                        dtt_args.append((CONST, int(bytecode[1])))
                elif bytecode_name == "LOADV":
                    if bytecode[1] in var_table.keys():
                        dtt_args.append((VAR, bytecode[1]))
                    else:
                        raise ParserError("Variable {} not known".format(bytecode[1]))
                elif bytecode_name == "LOADF":
                    dtt_args.append((FUNC, bytecode[1]))
                elif bytecode_name == "LOADT":
                    dtt_args.append((THREAD, bytecode[1]))
                else:
                    if bytecode_name not in ("CALL", "START") and len(FunctionTemplate.bytecodes[bytecode_name][1]) != previous_loads:
                        raise ParserError("Wrong number of loads before bytecode '{}'".format(''.join(bytecode)))
                    # todo test if corrent args
                    dtt.append(FunctionTemplate.bytecodes[bytecode_name][0])
                    previous_loads = 0

            bytecode_counter += 1

        return function_name, arg_table, var_table, dtt, dtt_args


class Thread(object):
    def __init__(self, function, thread_name, begin_y, begin_x):
        self.function = function
        self.recv_table = []
        self.status = None
        self.current_function = function
        self.name = thread_name

        self.window = curses.newwin(ThreadManager.window_height, ThreadManager.window_width, begin_y, begin_x)

    def refresh(self):
        thread_str = str(self.current_function)
        thread_str += " RECV:  {}\n".format(self.recv_table)
        self.window.clear()
        self.window.addstr(1,1,thread_str)
        self.window.box(0,0)
        self.window.refresh()

    def run(self):
        self.function.run()


class ThreadManager(object):
    window_width = 65
    window_height = 45

    def __init__(self, main_function):
        self.threads = []
        self.add(main_function, "MAIN_THREAD")
        self.current_thread = self.threads[0]


    def add(self, function, thread_name):
        begin_x = (len(self.threads) % 3) * ThreadManager.window_width + 5
        begin_y = (len(self.threads) // 3) * ThreadManager.window_height + 2
        thread = Thread(function, thread_name, begin_y, begin_x)
        self.threads.append(thread)
        thread.refresh()

    def remove(self, thread=None):
        if thread is None:
            self.threads.remove(self.current_thread)
            if len(self.threads) == 0:
                return self.exit()
            current_thread_index = randint(0, len(self.threads)-1)
            self.current_thread = self.threads[current_thread_index]
            self.current_thread.run()
        else:
            self.threads.remove(thread)

    def refresh(self):
        self.current_thread.refresh()

    def start(self):
        self.threads[0].run()

    def next(self):
        current_thread_index = self.threads.index(self.current_thread)
        current_thread_index = (current_thread_index + 1) % len(self.threads)
        self.current_thread = self.threads[current_thread_index]
        self.current_thread.run()

    def schedule(self):
        code_block_size = 4
        current_thread_index = self.threads.index(self.current_thread)
        cf = self.current_thread.current_function
        cf.dtt.insert(min(len(cf.dtt)+1, cf.vpc+code_block_size), vm_SCHEDULE)

        current_thread_index = (current_thread_index + 1) % len(self.threads)
        self.current_thread = self.threads[current_thread_index]
        self.current_thread.run()

    def exit(self):
        pass


class Program(object):
    def __init__(self, code_patch):
        self.terminal = curses.newwin(5, 100, curses.LINES - 1 - 5, 5)
        self.terminal.clear()
        self.terminal.addstr(1,1,"Terminal:\n")
        self.terminal.box(0,0)
        self.terminal.refresh()

        self.functions = [FunctionTemplate(open(code).read()) for code in glob(code_patch+'/*.pp')]
        function_names = [function.name for function in self.functions]

        if len(set(function_names)) != len(function_names):
            raise ParserError("Duplicate function names!")
        if "MAIN" not in function_names:
            raise ParserError("Not MAIN function!")

        main_function = list(filter(lambda x: x.name == "MAIN", self.functions))[0]
        self.thread_manager = ThreadManager(main_function())

    def print_terminal(self, data):
        self.terminal.clear()
        self.terminal.addstr(1,1, "Terminal:\n")
        self.terminal.addstr(2,1, str(data))
        self.terminal.box(0,0)
        self.terminal.refresh()

    def start(self):
        self.thread_manager.start()


if __name__ == "__main__":
    curses.initscr()
    curses.start_color()
    curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_WHITE)

    program = Program('example_program')
    program.start()
    
    
    