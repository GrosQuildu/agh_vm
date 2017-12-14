from random import randint
import curses

from functions import Function
from bytecode import vm_SCHEDULE


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
    window_width = 35
    window_height = 45

    def __init__(self, main_function):
        self.threads = []
        self.add(main_function, "MAIN_THREAD")
        self.current_thread = self.threads[0]


    def add(self, function, thread_name):
        begin_x = (len(self.threads) % 5) * ThreadManager.window_width + 5
        begin_y = (len(self.threads) // 5) * ThreadManager.window_height + 2
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
        self.schedule()

    def next(self):
        current_thread_index = self.threads.index(self.current_thread)
        current_thread_index = (current_thread_index + 1) % len(self.threads)
        self.current_thread = self.threads[current_thread_index]
        self.current_thread.run()

    def schedule(self):
        current_thread_index = self.threads.index(self.current_thread)
        cf = self.current_thread.current_function

        current_thread_index = (current_thread_index + 1) % len(self.threads)
        self.current_thread = self.threads[current_thread_index]
        self.current_thread.run()

    def insert_scheduler(self, function, position):
        function.dtt_removed.append(cf.dtt[position])
        function.dtt[position] = vm_SCHEDULE

    def fifs(self):
        pass

    def round_robin(self):
        code_block_size = 4
        cf = self.current_thread.current_function
        position = cf.vpc + code_block_size

        while cf is not None:
            if position >= len(cf.dtt):
                cf = cf.return_function
                position = cf.vpc + code_block_size
                continue
            insert_scheduler(cf, position)
            position += code_block_size

    def priority_based(self):
        pass

    def exit(self):
        pass