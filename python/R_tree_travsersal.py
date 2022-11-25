"""
This file reproduces the R-tree synchronized traversal process by
  recursive function call (the CPU style)
"""

import numpy as np

from Region import MBR
from RTree import Node

def join_nodes(node_A, node_B, results: list):
    """
    Results is the global result list of leaf intersections,
        each element of the list is a pair of object IDs
    """
    
    # One of the nodes is leaf
    if node_A.is_leaf and not node_B.is_leaf:
	    pass
    elif not node_A.is_leaf and node_B.is_leaf:
	    pass
    elif node_A.is_leaf and node_B.is_leaf: # both are leaf
        for i in range(node_A.get_count()):
            for j in range(node_B.get_count()):
                if node_A.mbrs[i].intersects(node_B.mbrs[j]):
                    if node_A.obj_ids[i] != node_B.obj_ids[j]:
                        results.append((node_A.obj_ids[i], node_B.obj_ids[j]))
    else: # neither is leaf
        for i in range(node_A.get_count()):
            for j in range(node_B.get_count()):
                if node_A.mbrs[i].intersects(node_B.mbrs[j]):
                    join_nodes(node_A.child_ptrs[i], node_B.child_ptrs[j], results)

if __name__ == '__main__':
    
    node = Node(is_leaf=True)
    node.add_entry(MBR(1,2,1,2), 10)
    mbr_list = [MBR(1,2,1,2)] * 5
    node.add_entries(mbr_list, [1,2,3,4,5])
    node.print_contents()

    results = []
    join_nodes(node, node, results)
    print("self join results: ", results)

    node = Node(is_leaf=False)
    node.add_entry(MBR(3,4,3,4), 12)
    mbr_list = [MBR(3,4,3,4)] * 5
    node.add_entries(mbr_list, [6,7,8,9,10])
    node.print_contents()

    