import numpy as np

from Index.Region import MBR
from Index.RTree import Node, sync_traversal, join_data_nodes
from Index.Tree_generation import generate_rtree

class FPGA_tree_traversal:

    level_tree_A = None 
    level_tree_B = None
    max_level = None 

    # number of results pairs in level cache, and the pointer position
    #   of the level (current_pointer_per_level[l] <= num_pairs_per_level[l])
    num_pairs_per_level = None
    current_pointer_per_level = None
    level_cache = None

    results = None

    def __init__(self, level_tree_A, level_tree_B):
        self.level_tree_A = level_tree_A 
        self.level_tree_B = level_tree_B
        self.max_level = np.amax([level_tree_A, level_tree_B])
        self.results = []

    def join_nodes_scheduler(self, root_A, root_B):
        """
        Recursively join two nodes in two R-trees. 

        Input: 
            node_A, node_B: two nodes in an R-tree
            results: the global result list of leaf intersections,
                each element of the list is a pair of object IDs
        Output: 
            None, the intersected pairs are appended in results
        """

        # The states of the scheduler
        current_level = 1 # the level where join happens, the results are pairs one level below
        num_pairs_per_level = dict()
        current_pointer_per_level = dict()
        level_cache = dict() # the results of level-l join == pairs of level l+1
        for i in range(1, self.max_level):
            num_pairs_per_level[i] = -1
            current_pointer_per_level[i] = 0 # set to differnet as num_pairs_per_level
            level_cache[i] = []

        # start from the root
        level_cache[current_level] = self.join_nodes(root_A, root_B)
        num_pairs_per_level[current_level] = len(level_cache[current_level])
        current_pointer_per_level[current_level] = 0
        if num_pairs_per_level[current_level] == 0:
            return []
        else:
            current_level += 1

        # in the previous iteration, whether traverse the tree from top to down or the opposite
        up_to_down = True 
        while True:

            # print("Current level: {}".format(current_level))
            
            # Run join at current_level
            #   the input pages are from level cache current_level - 1
            #   results are stored in level cache current_level (for bottom layer, store results)

            # end condition: root pair finished
            if current_pointer_per_level[1] == num_pairs_per_level[1]:
                break
            
            if up_to_down:
                # the input of current level comes from the last layer cache
                # print(len(level_cache[current_level - 1]), current_pointer_per_level[current_level - 1])
                # print(level_cache[current_level - 1], current_pointer_per_level[current_level - 1])
                node_A, node_B = level_cache[current_level - 1][current_pointer_per_level[current_level - 1]]

                # if at the bottom level: 
                if current_level == self.max_level:
                    self.results += self.join_nodes(node_A, node_B)
                    current_level -= 1
                    current_pointer_per_level[current_level] += 1
                    up_to_down = False
                else:
                    level_results = self.join_nodes(node_A, node_B)
                    if len(level_results) == 0: 
                        current_pointer_per_level[current_level] = 0
                        num_pairs_per_level[current_level] = -1
                        current_level -= 1
                        current_pointer_per_level[current_level] += 1
                        up_to_down = False
                    else: # start in the lower level 
                        level_cache[current_level] = level_results
                        num_pairs_per_level[current_level] = len(level_cache[current_level])
                        current_pointer_per_level[current_level] = 0
                        current_level += 1
                        up_to_down = True

            else: # down to up
                # not the root level (level 1) and not the bottom level (max level) 
                # if at the end of the current level cache -> move to the upper level, else continue join
                if current_pointer_per_level[current_level] == num_pairs_per_level[current_level]:
                    current_level -= 1
                    current_pointer_per_level[current_level] += 1
                    up_to_down = False
                else: 
                    current_level += 1
                    up_to_down = True
            

        return self.results

    def join_nodes(self, node_A, node_B):
        """
        Join two nodes.
        Output: a list of intersected pairs

        Note: for FPGA implementation, may need two types of PEs to handle this...
            one for entire page join; the other for an mbr joins a page
        """
        results = []

        if node_A.is_leaf and node_B.is_leaf: # result pairs: obj
            for i in range(node_A.get_count()):
                for j in range(node_B.get_count()):
                    if node_A.mbrs[i].intersects(node_B.mbrs[j]):
                        if node_A.obj_ids[i] != node_B.obj_ids[j]:
                            results.append((node_A.obj_ids[i], node_B.obj_ids[j]))
        elif node_A.is_leaf and not node_B.is_leaf: # result pairs: A and B's children
            for j in range(node_B.get_count()):
                if node_A.mbr.intersects(node_B.mbrs[j]):
                    results.append((node_A, node_B.child_ptrs[j]))
        elif not node_A.is_leaf and node_B.is_leaf: # result pairs: A's children and B
            for i in range(node_A.get_count()):
                if node_A.mbrs[i].intersects(node_B.mbr):
                    results.append((node_A.child_ptrs[i], node_B))
        else: # result pairs: A's children and B's children
            for i in range(node_A.get_count()):
                for j in range(node_B.get_count()):
                    if node_A.mbrs[i].intersects(node_B.mbrs[j]):
                        results.append((node_A.child_ptrs[i], node_B.child_ptrs[j]))

        return results

if __name__ == '__main__':

    level_tree_A = 4 
    level_tree_B = 2
    root_A = generate_rtree(max_level=level_tree_A, directory_node_fanout=2, data_node_fanout=100, root_mbr=None)
    root_B = generate_rtree(max_level=level_tree_B, directory_node_fanout=2, data_node_fanout=100, root_mbr=None)

    print("A join A:")
    results = sync_traversal(root_A, root_A)
    print("Result length: {}".format(len(results)))

    print("A join B:")
    results = sync_traversal(root_A, root_B)
    print("Result length: {}".format(len(results)))

    print("FPGA A join B:")
    traversal_instance = FPGA_tree_traversal(level_tree_A, level_tree_B)
    results = traversal_instance.join_nodes_scheduler(root_A, root_B)
    print("Result length: {}".format(len(results)))