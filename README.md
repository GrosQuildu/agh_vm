## Virtual Machine

A virtual machine developed during a Design Patterns course on AGH UST.

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


### Scheduler

Machine support three types of scheduling: first-in first-out, round-robin and priority based. Scheduler can be selected in VM command line arguments.

FIFO scheduler works as expected: main thread is executed, then next threads in order they were started. Blocking operations (recv, join) may force thread change.

Round robin executes few bytecodes from each thread, then move to next thread. Number of bytecodes is set by scheduler class in initialize method in Thread.cpp file.

Priority scheduler execute thread with the highest priority, if more than one thread have the same priority, they are scheduled in round-robin style.


### JIT

Machine had to works as >very simple< just-in-time compiler (or inline-threading as some call it). So each function is divided into blocks (size depends on which scheduler is used), each block is compiled by external compiler (gcc), loaded as shared library and called when needed. Blocks (c++ code as shared library file) are stored in folder specified in command line argument.


### Patterns

It was required to use at least three design patterns in the project. Comments in source code points to methods that implements a pattern.

* Observer
  + Used when joining thread (thread that want to wait register himself in target thread, when that thread ends, it notify each of registered threads)
  + Observers and observable classes are the same: Thread
* Singleton
  + VM class is used to start and clear after execution of bytecode. It also works as interface for bytecode for some instructions (like starting new thread). It's impemented as singleton.
  + Class: VM
* Prototype
  + Functions specified in ".pp" files can be reused (f.e. call the same funtion few times) and some of the variables (like consts and bytecode table) are constants and some vary (like function's arguments), so prototype pattern was used to simplify creation of new functions.
  + Classes: FunctionPrototype, FunctionFactory, Function.
* Strategy
  + Pseudo (lacks wrapper class) strategy pattern was used to implement different schedulers
  + Classes: ThreadScheduler, FIFOScheduler, RoundRobinScheduler, PriorityScheduler

