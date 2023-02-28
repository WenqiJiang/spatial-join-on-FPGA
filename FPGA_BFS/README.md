# Version control

This file tracks the functionality and performance of single/multi PE spatial join.

## Single PE
### V1 

The vanilla version of single PE, but already has all functionality. 
#### potential optimizations

1. The write functionalities uses a while loop thus cannot infer burst write.

Potential: add a write aggregation functionality, say there is N packages in the FIFO, so the write can be burst. 

#### Performance

Measure executor time.

1 PE:
* sample_tree_level_1_self_join_156 : 0.0138 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.170814 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 1.7227 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 21.9326 ms @ 140 MHZ

## Multi PE

### V1 

The vanilla version of multiple PE, but already has all functionality. 

#### potential optimizations

1. The design consumes the result from all join PEs in a round robin manner. In each round, it does a round-robin check for all join PEs in order to get results. This is true for both result write and cache manager. However, if one PE has much more results to write than the others, the process will be slow because reading the PE producing the most result will cost N_PE times. 

Potential solution: if the FIFO is not empty, use a while loop to continuously consume the data. 

2. The write functionalities uses a while loop thus cannot infer burst write.

Potential: add a write aggregation functionality, say there is N packages in the FIFO, so the write can be burst. 

#### Performance

Measure executor time.

1 PE:
* sample_tree_level_1_self_join_156 : 0.0138071 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.219321 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 2.34314 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 30.8454 ms @ 140 MHZ

**Conclusion: has some overhead compared to single PE version implementation. When there are many pages, the performance can only achieve ~70% of the single PE version.**