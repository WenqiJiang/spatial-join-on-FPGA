"""
Generate a random RTree
"""
import numpy as np

from Region import MBR
from RTree import Node

def random_sub_MBR(mbr):
    """
    Given an mbr, return a random sub-mbr
    """
    low0 = mbr.get_low0()
    high0 = mbr.get_high0()
    low1 = mbr.get_low1()
    high1 = mbr.get_high1()

    delta_0 = high0 - low0
    delta_1 = high1 - low1
    r0_low, r0_high = np.sort(np.random.randint(1, 100, (2)) / 100.0)
    r1_low, r1_high = np.sort(np.random.randint(1, 100, (2)) / 100.0)

    new_low0 = low0 + r0_low * delta_0
    new_high0 = low0 + r0_high * delta_0
    new_low1 = low1 + r1_low * delta_1
    new_high1 = low1 + r1_high * delta_1

    sub_mbr = MBR(new_low0, new_high0, new_low1, new_high1)

    return sub_mbr


def generate_rtree(max_level=2, directory_node_fanout=2, data_node_fanout=100, root_mbr=None):

    assert max_level >= 2, "Level must be larger than 2 (root is a level itself)"
    if root_mbr is None:
        root_mbr = MBR(0, 100, 0, 100)
    
    # create root node
    root_node = Node(is_leaf=False, mbr=root_mbr)

    class TreeGenerator:

        global_obj_counter = None

        def __init__(self):
            self.global_obj_counter = 0

        def generate_recursive(self, node, current_level, max_level, directory_node_fanout, data_node_fanout):

            if current_level == max_level: # attach data in the input data node
                # print("Max Level")
                for i in range(data_node_fanout):
                    node.add_entry(random_sub_MBR(node.mbr), self.global_obj_counter)
                    self.global_obj_counter += 1
            else: # add child nodes
                for i in range(directory_node_fanout):
                    child_mbr = random_sub_MBR(node.mbr)
                    if current_level == max_level - 1: # child is leaf
                        child_node = Node(is_leaf=True, mbr=child_mbr) 
                    else: # child is directory
                        child_node = Node(is_leaf=False, mbr=child_mbr) 
                    node.add_entry(child_mbr, child_node)
                    self.generate_recursive(child_node, current_level + 1, max_level, directory_node_fanout, data_node_fanout)

    tree_generator = TreeGenerator()
    tree_generator.generate_recursive(root_node, 1, max_level, directory_node_fanout, data_node_fanout)

    return root_node


if __name__ == "__main__":
	
    max_level = 4
    directory_node_fanout=2
    data_node_fanout=100
    root_mbr=None

    root_node = generate_rtree(max_level, directory_node_fanout, data_node_fanout, root_mbr)

    current_node = root_node
    for i in range(1, max_level + 1):
        print("Level ", i)
        current_node.print_contents()
        if i < max_level:
            current_node = current_node.child_ptrs[current_node.get_count() - 1]