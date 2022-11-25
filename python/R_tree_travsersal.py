"""
This file reproduces the R-tree synchronized traversal process by
  recursive function call (the CPU style)
"""

import numpy as np

class MBR:
    """
    Refer to Region2D: https://github.com/WenqiJiang/CPU-spatial-join/blob/main/include/2D/Region2D.h
    """

    low0 = None
    high0 = None
    low1 = None 
    high1 = None

    def __init__(self, low0 : float, high0 : float, low1 : float, high1 : float):
        self.low0 = low0
        self.high0 = high0
        self.low1 = low1
        self.high1 = high1

    def get_low0(self): return self.low0

    def get_high0(self): return self.high0

    def get_low1(self): return self.low1
    
    def get_high1(self): return self.high1

    def intersects(self, other):
        # whether the current MBR intersects another MBR
        if ((self.get_low0() <= other.get_high0() and self.get_high0 >= other.get_low0()) and 
            (self.get_low1 <= other.get_high1() and self.get_high1 >= other.get_low1())):
            return True
        else:
            return False


class Node:
    """
    Refer to R-tree Node: https://github.com/WenqiJiang/CPU-spatial-join/blob/main/include/2D/point_index/RTree.h
    """
    is_leaf = False
    count = 0

    child_ptrs = None # for inodes
    obj_ids = None # for leaf nodes

    def __init__(self, is_leaf : bool):
        self.is_leaf = is_leaf
        if not is_leaf:
            self.child_ptrs = [] # int list
        else:
            self.obj_ids = [] # int list

    def print_contents(self):
        print("Is leaf: ", self.get_count())
        print("Number of items: ", self.count)
        print("Children pointers: ", self.child_ptrs)
        print("Object IDs: ", self.obj_ids)

    def get_count(self):
        return self.count

    def add_entry(self, data : int):
        if not self.is_leaf:
            self.child_ptrs.append(data)
        else:
            self.obj_ids.append(data)
        self.count += 1

    def add_entries(self, data: list):
        if not self.is_leaf:
            for d in data:
                self.child_ptrs.append(d)
        else:
            for d in data:
                self.obj_ids.append(d)
        self.count += len(data)

    

def join_nodes(node_A : Node, node_B : Node):
	pass

if __name__ == '__main__':
    
    node = Node(is_leaf=True)
    node.add_entry(10)
    node.add_entries([1,2,3,4,5])
    node.print_contents()

    node = Node(is_leaf=False)
    node.add_entry(12)
    node.add_entries([6,7,8,9,10])
    node.print_contents()