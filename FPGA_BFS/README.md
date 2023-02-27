# Version control

This file tracks the functionality and performance of single/multi PE spatial join.

## Single PE

## Multi PE

### V1 

The vanilla version of multiple PE, but already has all functionality. 

#### potential optimizations

1. The design consumes the result from all join PEs in a round robin manner. In each round, it does a round-robin check for all join PEs in order to get results. This is true for both result write and cache manager. However, if one PE has much more results to write than the others, the process will be slow because reading the PE producing the most result will cost N_PE times. 

Potential solution: if the FIFO is not empty, use a while loop to continuously consume the data. 

2. The write functionalities uses a while loop thus cannot infer burst write.

Potential: add a write aggregation functionality, say there is N packages in the FIFO, so the write can be burst. 

#### Performance. 