"""
Generate a random RTree
"""
import numpy as np

from Index.Region import MBR
from Index.RTree import Node

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
    """
    Return the root node of a random R-tree generated with the input arguments
    """

    assert max_level >= 2, "Level must be larger than 2 (root is a level itself)"
    if root_mbr is None:
        root_mbr = MBR(0, 100, 0, 100)
    
    # create root node
    root_node = Node(node_id=0, is_leaf=False, mbr=root_mbr)

    class TreeGenerator:

        global_obj_counter = None
        global_nodes_counter = None # pages

        def __init__(self, global_nodes_counter=1):
            # root node already counts one by default
            self.global_obj_counter = 0
            self.global_nodes_counter = global_nodes_counter

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
                        child_node = Node(node_id=self.global_nodes_counter, is_leaf=True, mbr=child_mbr) 
                    else: # child is directory
                        child_node = Node(node_id=self.global_nodes_counter, is_leaf=False, mbr=child_mbr) 
                    self.global_nodes_counter += 1
                    node.add_entry(child_mbr, child_node)
                    self.generate_recursive(child_node, current_level + 1, max_level, directory_node_fanout, data_node_fanout)

    tree_generator = TreeGenerator(global_nodes_counter=1)
    tree_generator.generate_recursive(root_node, 1, max_level, directory_node_fanout, data_node_fanout)

    return root_node

def collect_all_nodes(root):
    """
    Run BFS starting from the root, and generate the list of all nodes in the tree, 
        whose node IDs are consecutively increasing  
    """
    candidate_node_list = [] # as a queue
    final_node_list = [] # as the final node list

    candidate_node_list.append(root) 
    while len(candidate_node_list) != 0:
        current_node = candidate_node_list.pop(0)
        final_node_list.append(current_node)
        if not current_node.is_leaf:
            candidate_node_list += current_node.child_ptrs

    # the nodes are generated in recusive fashion, thus need to sort these nodes by IDs
    final_node_list.sort(key=lambda n: n.node_id)
    for i in range(len(final_node_list)):
        assert final_node_list[i].node_id == i

    return final_node_list

def index_serialization(root, node_bytes=4096, out_dir=None):
    """
    Inputs: 
        a root node
        all the FPGA related parameters 
            node_bytes: the size per page for a single node
        output file directory: no file write without specifying 
        
    FPGA storage format:
        one node per page (e.g., 4096 bytes)
        data are split in 64-byte AXI width 

        first 64 byte:

            typedef struct {
                // 7 * 4 bytes = 28 bytes
                int is_leaf;  // bool 
                int count;    // valid items
                obj_t obj;    // id/ptr + mbr
            } node_meta_t;

        the rest 64-byte blocks, each contain 3 object (first 60 bytes):

            typedef struct {
                // obj id for data nodes; pointer to children for directory nodes
                int id; 
                // minimum bounding rectangle 
                float low0; 
                float high0; 
                float low1; 
                float high1; 
            } obj_t;

    Output: 
        a binary file format of the tree
    """
    node_list = collect_all_nodes(root)
    num_nodes = len(node_list)

    def node_serialization(node, node_bytes=4096):

        node_bin = bytes()

        empty_byte = int(0).to_bytes(1, "little", signed=False)
        AXI_bytes = 64
        obj_bytes = 20
        header_bytes = 28

        # meta data: 28 byte out of the first 64
        if node.is_leaf:
            node_bin += int(1).to_bytes(4, "little", signed=False)
        else:
            node_bin += int(0).to_bytes(4, "little", signed=False)

        node_bin += int(node.count).to_bytes(4, "little", signed=False)

        node_bin += int(node.node_id).to_bytes(4, "little", signed=False)

        node_bin += int(node.mbr.get_low0()).to_bytes(4, "little", signed=False)
        node_bin += int(node.mbr.get_high0()).to_bytes(4, "little", signed=False)
        node_bin += int(node.mbr.get_low1()).to_bytes(4, "little", signed=False)
        node_bin += int(node.mbr.get_high1()).to_bytes(4, "little", signed=False)

        assert len(node_bin) == header_bytes
        node_bin += (AXI_bytes - header_bytes) * empty_byte

        # handle all the children

        # for data node, fill id as polygon id
        if node.is_leaf:

            assert node.count == len(node.obj_ids)
            assert node.count == len(node.mbrs)

            for i in range(int(np.ceil(node.count / 3.0))):
                # append 3 objects, then fill 4 bytes
                for j in range(3):
                    child_id = i * 3 + j
                    if child_id >= node.count:
                        break
                    node_bin += int(node.obj_ids[i]).to_bytes(4, "little", signed=False)
                    node_bin += int(node.mbrs[i].get_low0()).to_bytes(4, "little", signed=False)
                    node_bin += int(node.mbrs[i].get_high0()).to_bytes(4, "little", signed=False)
                    node_bin += int(node.mbrs[i].get_low1()).to_bytes(4, "little", signed=False)
                    node_bin += int(node.mbrs[i].get_high1()).to_bytes(4, "little", signed=False)
                node_bin += (AXI_bytes - 3 * obj_bytes) * empty_byte
        # for directory node, fill id as children node id
        else: # is directory
            assert node.count == len(node.child_ptrs)
            assert node.count == len(node.mbrs)

            for i in range(int(np.ceil(node.count / 3.0))):
                # append 3 objects, then fill 4 bytes
                for j in range(3):
                    child_id = i * 3 + j
                    if child_id >= node.count:
                        break
                    node_bin += int(node.child_ptrs[i].node_id).to_bytes(4, "little", signed=False)
                    node_bin += int(node.child_ptrs[i].mbr.get_low0()).to_bytes(4, "little", signed=False)
                    node_bin += int(node.child_ptrs[i].mbr.get_high0()).to_bytes(4, "little", signed=False)
                    node_bin += int(node.child_ptrs[i].mbr.get_low1()).to_bytes(4, "little", signed=False)
                    node_bin += int(node.child_ptrs[i].mbr.get_high1()).to_bytes(4, "little", signed=False)
                node_bin += (AXI_bytes - 3 * obj_bytes) * empty_byte

        # final assertion & padding
        if len(node_bin) > node_bytes:
            raise ValueError
        if len(node_bin) < node_bytes:
            node_bin += (node_bytes - len(node_bin)) * empty_byte
        assert len(node_bin) == node_bytes

        return node_bin
        
    # assert that the nodes are sorted by IDs
    for i in range(num_nodes):
        assert node_list[i].node_id == i

    tree_bin = bytes()
    for i in range(num_nodes):
        tree_bin += node_serialization(node_list[i], node_bytes)

    assert len(tree_bin) == num_nodes * node_bytes

    if out_dir is not None: 
        with open(out_dir, 'wb') as f:
            f.write(tree_bin)

    return tree_bin

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