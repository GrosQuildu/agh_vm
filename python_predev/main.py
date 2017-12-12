#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
~PP
'''


import curses

from virtual_machine import Program
from context import init_context


if __name__ == "__main__":
    curses.initscr()
    curses.start_color()
    curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_WHITE)

    program = Program('example_program')
    init_context(program)
    program.start()
    
    
    