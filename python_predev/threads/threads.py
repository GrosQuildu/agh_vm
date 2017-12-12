from random import randint
import curses

from functions import Function


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