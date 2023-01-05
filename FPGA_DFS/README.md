# FPGA DFS 

## DFS_single_PE

The entire DFS implementation without any performance optimization. 

This contains two kernels, a scheduler and an executor passing data to each other. 

## DFS_single_PE_DEBUG

A very simple skeleton of DFS, verifying that passing data between module can be done. 

This contains two kernels, a scheduler and an executor passing data to each other. 

## Unused

### dataflow_version_deadlock

Dataflow implementations of the DFS, which has feedback loops leading to deadlock.