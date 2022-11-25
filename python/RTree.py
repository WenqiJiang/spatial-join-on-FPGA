
class Node:
    """
    Refer to R-tree Node: https://github.com/WenqiJiang/CPU-spatial-join/blob/main/include/2D/point_index/RTree.h
    """
    is_leaf = False
    count = 0

    child_ptrs = None # for inodes
    obj_ids = None # for leaf nodes
    mbrs = None

    def __init__(self, is_leaf : bool):
        self.is_leaf = is_leaf
        if not is_leaf:
            self.child_ptrs = [] # int list
        else:
            self.obj_ids = [] # int list
        self.mbrs = []

    def print_contents(self):
        print("Is leaf: ", self.get_count())
        print("Number of items: ", self.count)
        print("Children pointers: ", self.child_ptrs)
        print("Object IDs: ", self.obj_ids)
        print("MBR list: ", self.mbrs)

    def get_count(self):
        return self.count

    def add_entry(self, mbr, data : int):
        if not self.is_leaf:
            self.child_ptrs.append(data)
        else:
            self.obj_ids.append(data)
        self.mbrs.append(mbr)
        self.count += 1

    def add_entries(self, mbrs : list, data: list):
        assert len(mbrs) == len(data)
        if not self.is_leaf:
            for d in data:
                self.child_ptrs.append(d)
        else:
            for d in data:
                self.obj_ids.append(d)
        for mbr in mbrs:
            self.mbrs.append(mbr)
        self.count += len(data)

    
    # void SynchTraversal<U,K,A,RG>::joinNodes(JoinCallBack<K,U,U>* cb, 
    #                                          RNode* p, 
    #                                          RNode* q) {
    #     if(p->isLeaf() && !q->isLeaf()) {
    #         for(int i = 0; i < p->getCount(); i++) {
    #             joinNodeRegion(cb, q, p->mbrs[i], p->ptrs[i].data, true);
    #         }
    #     }
    #     else if(!p->leaf && q->leaf) {
    #         for(int i = 0; i < q->count; i++) {
    #             joinNodeRegion(cb, p, q->mbrs[i], q->ptrs[i].data, false);
    #         }
    #     }
    # }