## Virtual Machine

Project for AGH subject design patterns.

### Build and run:

```bash
bash ./build.sh [ARM | x86] [NORMAL | DEBUG]

qemu-arm -L /usr/arm-linux-gnueabi/ ./VM --help
# or
./VM --help
```

DEBUG mode uses ncurses.


### Bytecode
Program to run must be placed in separate directory. Files extension must be ".pp".

One file is one function. Template is:

```
DEF FUNCTION_NAME ARGS_COUNT
DECLARE VARIABLE1
DECLARE VARIABLE2
...

LOAD 0
RETURN
END
```

Variables declaration must be placed just after function header. Variables can store signed integers. Functions must end with RETURN and END.

Machine implements (software) threading, each thread must be started with a function. Main thread is determined by function name: "MAIN".

Word after bytecode can be one of: VAR (variable name), CONST (integer), ARG (ARG_number, like ARG_0), FUNC (function name), THREAD (thread name)

Arguments to bytecode (like RETURN or ADD) are passed by LOAD[V|T|F] bytecode.


##### Assign, print, arithmetic

```
# varname := 12
LOADV VARNAME
LOAD 12
ASSIGN

# print args[1]
LOAD ARG_1
PRINT

# var1 := var2 + 12
LOADV VAR1
LOADV VAR2
LOAD 12
ADD

# var1 := var1 - var2
LOADV VAR1
LOADV VAR1
LOADV VAR2
SUB

# var1 := 12 / arg[0]
LOADV VAR1
LOAD 12
LOAD ARG_0
DIV

# var2 := var1  * var3
LOADV VAR2
LOADV VAR1
LOADV VAR3
MUL
```

##### Function calls

```
# var1 := function_name(-15, var1)
LOADF FUNCTION_NAME  # function to call, name as declared in function header after "DEF"
LOADV VAR1  # variable where to store result of function call
LOAD -15  # first arg to function, amount of args for function is declared in function header after function name
LOADV VAR1  # second arg to function
CALL

# return -1
LOAD -1
RETURN
```

##### Threads

```
# new thread, return value of thread's main function will be lost
LOADF FUNC_NAME
LOADT THREAD_NAME  # must be unique in whole program
LOAD -15  # first argument to function

# stop thread
LOADT THREAD_NAME
STOP

# join (wait until given thread ends)
LOADT THREAD_TO_WAIT_FOR_NAME
JOIN

# send value to thread, non blocking call
LOADT THREAD_NAME
LOADV VAR1
SEND

# recv value, store in varx. Blocks until any value in recv table. Can't determine which thread send value.
LOADV VARX
RECV

# change thread's priority, must be in range [1,10]. Greater value, more priority.
LOADT THREAD_NAME
LOAD 9
PRIORITY
```

Examples in `example_programs` directory



