# Version control

This file tracks the functionality and performance of single/multi PE spatial join.

## Single PE
### V1.0

The vanilla version of single PE, but already has all functionality. 
#### potential optimizations

1. The write functionalities uses a while loop thus cannot infer burst write.

Potential: add a write aggregation functionality, say there is N packages in the FIFO, so the write can be burst. 

#### Performance

Measure executor time.

* sample_tree_level_1_self_join_156 : 0.0138 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.170814 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 1.7227 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 21.9326 ms @ 140 MHZ

### V2.0

Optimize the write functionality, manually do write bursting, and the AXI burst length as max_write_burst_length=128 (which has been proved to be sufficient in my write microbenchmarks).

#### Performance

Measure executor time.

* sample_tree_level_1_self_join_156 : 0.00781429 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.0749929 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 0.915336 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 12.7479 ms @ 140 MHZ

Conclusion: 2X speedup over the while loop write version. 
### V2.1

Further improve write burst: using a long burst packet size of 1024 and a shorter AXI burst length of 64 to allow multiple write operations on the fly. 

#### Performance

Measure executor time.
* sample_tree_level_1_self_join_156 : 0.00779286 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.0729286 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 0.915279 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 12.7375 ms @ 140 MHZ

Conclusion: No improvement. Pretty much the same as V2.0.
## Multi PE

### V1.0 

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

8 PE:
* sample_tree_level_1_self_join_156 : 0.0157857 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.194357 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 1.85011 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 20.6116 ms @ 140 MHZ

**Conclusion: has some speedup compare to 1 PE, but not a lot probably due to (a) the inefficient result gather functionality (b) the selectivity is too low, such that most of the pairs are written to memory which caps the performance.**


### V1.1

Compare to 1.0, optimizing the round-robin read from join PEs. Basically, use a while to read from a PE, so if one PE has much more output than others, the writer / cache manager can keep reading from it. 

Measure executor time.

1 PE:
* sample_tree_level_1_self_join_156 : 0.0138 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.2079 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 2.18251 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 28.7811 ms @ 140 MHZ

**Conclusion: better than V1.0, but still has some overhead compared to single PE version implementation. When there are many pages, the performance can only achieve ~76% of the single PE version.**


8 PE:
* sample_tree_level_1_self_join_156 : 0.0158286 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.173543 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 1.68542 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 21.2492 ms @ 140 MHZ

**Conclusion: has some speedup compare to 1 PE, but not a lot probably due to the selectivity is too low, such that most of the pairs are written to memory which caps the performance. The gather functionality should not be the problem anymore.**


### V2.0

Compare to 1.0, added the burst write functionality, and the AXI burst length as max_write_burst_length=128 (which has been proved to be sufficient in my write microbenchmarks).

1 PE:
* sample_tree_level_1_self_join_156 : 0.00977143 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.120871 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 1.46261 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 20.2549 ms @ 140 MHZ

**Conclusion: better than V1.0, ~1.5X speedup.**

8 PE:
* sample_tree_level_1_self_join_156 : 0.00979286 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.0674714 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 0.719693 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 8.99693 ms @ 140 MHZ

**Conclusion: better than V1.0, ~2X speedup; better than 1 PE, ~2X speedup -> but not 8X, probably need to use larger workloads to examine the performance.**
### V2.1

Compare to 2.0, further optimize the burst write functionality (wait until either a page join is finished or until the results are enough for a full burst).


1 PE:
* sample_tree_level_1_self_join_156 : 0.00980714 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.120879 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 1.46091 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 20.2526 ms @ 140 MHZ
* sample_tree_level_5_self_join_5182308.bin: 313.612 ms @ 140 MHZ

**Conclusion: essentially the same as V2.0.**

8 PE:
* sample_tree_level_1_self_join_156 : 0.00982143 ms @ 140 MHZ
* sample_tree_level_2_self_join_2090.bin : 0.0614286 ms @ 140 MHZ
* sample_tree_level_3_self_join_19246.bin : 0.649243 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 8.88854 ms @ 140 MHZ
* sample_tree_level_5_self_join_5182308.bin: 147.577 ms @ 140 MHZ

**Conclusion: essentially the same as V2.0. The speedup over 1 PE version is always slightly more than 2X, no matter the workload, so there is some scalability issue here.**

### V2.2

Compare to 2.1: 
* Add a monitor detecting the number of pages allocated to each PE.
* Fix the write address, the previous version writes from addr 0 which will be overwritten by result count...


8 PE:
* sample_tree_level_5_self_join_5182308.bin: 147.552 ms @ 140 MHZ

PE 0 computes 12039 page joins
PE 1 computes 11999 page joins
PE 2 computes 11734 page joins
PE 3 computes 11501 page joins
PE 4 computes 11562 page joins
PE 5 computes 11522 page joins
PE 6 computes 11422 page joins
PE 7 computes 11398 page joins

**Conclusion: the workloads are quite balanced across PEs -> so they are indeed busy most of the time (otherwise PE 0 will be dispatched to the job). So there must be something else preventing the scalability, e.g., is the write still too intensive?**

An estimation: 5182308 / (12039 * 8 * 256) = 21\% -> this does not even count the intermediate result writes. So having a higher selectivity will improve the performance much more.

Writing 5182308 results, if we can achieve 1 write per cycle, consume 5182308 / (140 * 1e6) = 0.037 sec = 37 ms, without considering intermediate result writes. 

### V2.3

Further improve write burst: using a long burst packet size of 1024 and a shorter AXI burst length of 64 to allow multiple write operations on the fly. 

1 PE:

* sample_tree_level_3_self_join_19246.bin : 1.46256 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 20.2534 ms @ 140 MHZ
* sample_tree_level_5_self_join_5182308.bin: 313.537 ms @ 140 MHZ

8 PE:

* sample_tree_level_3_self_join_19246.bin : 0.682864 ms @ 140 MHZ
* sample_tree_level_4_self_join_235112.bin : 8.89053 ms @ 140 MHZ
* sample_tree_level_5_self_join_5182308.bin: 137.229 ms @ 140 MHZ

**Conclusion: no improvement after optimizing write. The problem could very well be the read: the read unit has to handle too many pages, because each only contains too little content (16 MBRs per page in the current workload). Plus, the layer cache has to fetch data pretty frequently from memory.**
#### Potential further optimization

* optimizize scheduler protocol: it should cache a number of page pairs to assign next, rather than issuing a lot of read request to the cache manager.
* change the workload -> it should join pages of at least hundreds of entries, not using the current workload with just 16 MBRs. 