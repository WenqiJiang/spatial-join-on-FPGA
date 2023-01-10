"""
Given an constructed RTree, serialize it into the format that FPGAs will operate on
"""
import numpy as np

from Index.Region import MBR
from Index.RTree import Node
from R_tree_traversal import sync_traversal
from Index.Tree_generation import generate_rtree, collect_all_nodes, index_serialization

if __name__ == '__main__':

    print("\n\nRunning serialization on generated R trees:")
    max_level = 4
    directory_node_fanout = 8
    data_node_fanout = 16
    root_A = generate_rtree(max_level=max_level, directory_node_fanout=directory_node_fanout, data_node_fanout=data_node_fanout, root_mbr=None)


    results = sync_traversal(root_A, root_A)
    print("Result length: {}".format(len(results)))

    candidate_node_list = collect_all_nodes(root_A)
    print("Node list length: {}".format(len(candidate_node_list)))
    fname = '/mnt/scratch/wenqi/spatial-join-on-FPGA/tree_bin/sample_tree_level_{}_self_join_{}.bin'.format(max_level, len(results))
    tree_bin = index_serialization(root_A, node_bytes=4096, out_dir=fname)
    print("Entire tree bytes: {}".format(len(tree_bin)))