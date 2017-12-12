from context import get_context, get_virtual_machine


TOKEN_TYPES = {
    "VAR": 0,
    "ARG": 1,
    "CONST": 2,
    "FUNC": 3,
    "THREAD": 4,
}
def const_n2s(number):
    for k, v in TOKEN_TYPES.iteritems():
        if v == number:
            return k
    return ''


def vm_DECLARE():
    pass

def vm_LOAD():
    pass

def vm_SCHEDULE():
    cf = get_context().current_function
    cf.vpc += 1

    get_virtual_machine().thread_manager.schedule()

def vm_ASSIGN():
    cf = get_context().current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    cf.dtt_args = cf.dtt_args[2:]

    if arg1_type == TOKEN_TYPES["VAR"]:
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
    function_to_call_template = list(filter(lambda x: x.name == arg0_val, get_virtual_machine().functions))
    if len(function_to_call_template) < 1:
        raise ParserError("Function {} not known".format(arg0_val))
    else:
        function_to_call_template = function_to_call_template[0]

    args_count = len(function_to_call_template.arg_table)
    args = [cf.dtt_args[i] for i in range(args_count)]
    args = map(lambda arg: cf.var_table[arg[1]] if arg[0] == TOKEN_TYPES["VAR"] else arg[1], args)
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

    if arg0_type == TOKEN_TYPES["VAR"]:
        arg0_val = cf.var_table[arg0_val]

    if cf.return_function is None:
        get_virtual_machine().thread_manager.remove()
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

    if arg1_type == TOKEN_TYPES["VAR"]:
        arg1_val = cf.var_table[arg1_val]

    thread_to_send = list(filter(lambda x: x.name == arg0_val, get_virtual_machine().thread_manager.threads))
    if len(thread_to_send) == 1:
        thread_to_send[0].recv_table.append((ct.name, arg1_val))

    cf.vpc += 1
    cf.dtt[cf.vpc]()

def vm_RECV():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    
    if len(ct.recv_table) == 0:
        get_virtual_machine().thread_manager.next()
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
    function_to_call_template = list(filter(lambda x: x.name == arg0_val, get_virtual_machine().functions))
    if len(function_to_call_template) < 1:
        raise ParserError("Function {} not known".format(arg0_val))
    else:
        function_to_call_template = function_to_call_template[0]

    args_count = len(function_to_call_template.arg_table)
    args = [cf.dtt_args[i] for i in range(args_count)]
    args = map(lambda arg: cf.var_table[arg[1]] if arg[0] == TOKEN_TYPES["VAR"] else arg[1], args)
    cf.dtt_args = cf.dtt_args[args_count:]
    function_to_call = function_to_call_template(*args)

    get_virtual_machine().thread_manager.add(function_to_call, arg1_val)

    cf.vpc += 1
    cf.dtt[cf.vpc]()


def vm_JOIN():
    ct = get_context()
    cf = ct.current_function
    arg0_type, arg0_val = cf.dtt_args[0]

    thread_to_join = list(filter(lambda x: x.name == arg0_val, get_virtual_machine().thread_manager.threads))
    if len(thread_to_join) == 0:
        cf.dtt_args = cf.dtt_args[1:]
        cf.vpc += 1
        cf.dtt[cf.vpc]()
    else:
        get_virtual_machine().thread_manager.next()

def vm_STOP():
    pass

def vm_ADD():
    cf = get_context().current_function
    arg0_type, arg0_val = cf.dtt_args[0]
    arg1_type, arg1_val = cf.dtt_args[1]
    arg2_type, arg2_val = cf.dtt_args[2]
    cf.dtt_args = cf.dtt_args[3:]

    if arg1_type == TOKEN_TYPES["VAR"]:
        arg1_val = cf.var_table[arg1_val]
    if arg2_type == TOKEN_TYPES["VAR"]:
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
    get_virtual_machine().print_terminal(cf.var_table[arg0_val])

    cf.vpc += 1
    cf.dtt[cf.vpc]()

bytecodes_table = {
    "DECLARE": (vm_DECLARE, [TOKEN_TYPES["VAR"]]),
    "LOAD": (vm_LOAD, [(TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "LOADV": (vm_LOAD, [TOKEN_TYPES["VAR"]]),
    "LOADF": (vm_LOAD, [TOKEN_TYPES["FUNC"]]),
    "LOADT": (vm_LOAD, [TOKEN_TYPES["THREAD"]]),
    "ASSIGN": (vm_ASSIGN, [TOKEN_TYPES["VAR"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "DEF": (vm_DEF, [TOKEN_TYPES["FUNC"], TOKEN_TYPES["CONST"]]),
    "END": (vm_END, []),
    "CALL": (vm_CALL, [TOKEN_TYPES["FUNC"], TOKEN_TYPES["VAR"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "RETURN": (vm_RETURN, [(TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "SEND": (vm_SEND, [TOKEN_TYPES["THREAD"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "RECV": (vm_RECV, [TOKEN_TYPES["VAR"]]),
    "START": (vm_START, [TOKEN_TYPES["FUNC"], TOKEN_TYPES["THREAD"]]),
    "JOIN": (vm_JOIN, [TOKEN_TYPES["THREAD"]]),
    "STOP": (vm_STOP, [TOKEN_TYPES["THREAD"]]),
    "ADD": (vm_ADD, [TOKEN_TYPES["VAR"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"]), (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "SUB": (vm_SUB, [TOKEN_TYPES["VAR"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"]), (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "DIV": (vm_DIV, [TOKEN_TYPES["VAR"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"]), (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "MUL": (vm_MUL, [TOKEN_TYPES["VAR"], (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"]), (TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])]),
    "PRINT": (vm_PRINT, [(TOKEN_TYPES["VAR"], TOKEN_TYPES["CONST"], TOKEN_TYPES["ARG"])])
}