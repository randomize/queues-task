
import gdb
import gdb.printing
# from curses.ascii import isgraph

class node_tPrinter(object):
    "Print a node_t"
    
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return ("d=" + str(self.val["data"]) + " n=<" + str(self.val["nxt"]) + ">")

    def display_hint(self):
        return 'string'

def str_lookup_function(val):
    lookup_tag = val.type.tag
    if lookup_tag == None:
        return None
    regex = re.compile("^node_t$")
    if regex.match(lookup_tag):
        return node_tPrinter(val)
    return None

gdb.pretty_printers.append(str_lookup_function)


