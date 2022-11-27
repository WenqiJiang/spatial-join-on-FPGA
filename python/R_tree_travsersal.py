"""
This file reproduces the R-tree synchronized traversal process by
  recursive function call (the CPU style)
"""

import numpy as np

from Index.Region import MBR
from Index.RTree import Node, sync_traversal, join_data_nodes


if __name__ == '__main__':
    
    node = Node(is_leaf=True)
    node.add_entry(MBR(1,2,1,2), 10)
    mbr_list = [MBR(1,2,1,2)] * 5
    node.add_entries(mbr_list, [1,2,3,4,5])
    node.print_contents()

    results = []
    results = sync_traversal(node, node)
    print("self join results ({} pairs): {}".format(len(results), results))
    results = join_data_nodes(node, node)
    print("self join results ({} pairs): {}".format(len(results), results))

    node = Node(is_leaf=False)
    node.add_entry(MBR(3,4,3,4), 12)
    mbr_list = [MBR(3,4,3,4)] * 5
    node.add_entries(mbr_list, [6,7,8,9,10])
    node.print_contents()

    