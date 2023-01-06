# FPGA DFS 

### DFS_single_PE_block_read

The entire DFS implementation without any performance optimization. 

This contains two kernels, a scheduler and an executor passing data to each other. 

Using explicit block read. 

(Compilation failure during routing, seems something went wrong from HLS level...)

### DFS_single_PE_test_interface

This contains two kernels, a scheduler and an executor passing data to each other. 

Only passing one signal to each other.

Using explicit block read. 

Succeed.

### DFS_single_PE_test_interface_with_executor_PEs

This contains two kernels, a scheduler and an executor passing data to each other. 

Only passing one signal to each other. 

The internal FIFO of the executer are not used (no read/write).

The executor contains several sub-PEs, but the internal FIFOs are not used. 

Succeed. 

### DFS_single_PE_test_interface_with_executor_PEs_FIFOs

This contains two kernels, a scheduler and an executor passing data to each other. 

Only passing one signal to each other.

The internal FIFO of the executer are also used (read/write once per FIFO).

The executor contains several sub-PEs, and the internal FIFOs are not used. 

Succeed.
## plain_read_deadlock
### DFS_single_PE

The entire DFS implementation without any performance optimization. 

This contains two kernels, a scheduler and an executor passing data to each other. 

### DFS_single_PE_DEBUG

A very simple skeleton of DFS, verifying that passing data between module can be done. 

This contains two kernels, a scheduler and an executor passing data to each other. 

## Unused

### dataflow_version_deadlock

Dataflow implementations of the DFS, which has feedback loops leading to deadlock.