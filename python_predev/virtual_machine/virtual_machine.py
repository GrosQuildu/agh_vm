import curses
from glob import glob

from threads import ThreadManager
from functions import FunctionTemplate


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
