from time import sleep


def init_context(new_virtual_machine):
    global virtual_machine
    virtual_machine = new_virtual_machine


def get_context():
    ct = virtual_machine.thread_manager.current_thread
    ct.refresh()
    sleep(1)
    return ct


def get_virtual_machine():
    return virtual_machine
