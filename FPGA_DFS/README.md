# FPGA DFS 

### DFS_single_PEd (debugging...)

The entire DFS implementation without any performance optimization. 

This contains two kernels, a scheduler and an executor passing data to each other. 

Using explicit block read. 

Sometimes the routing fails, complaining some verification issue. 

This can be solved by assigning the kernels to specific SLRs (routing in SLR 1 is typically problematic, because the static region sits there and consumes half of the SLR). 

Example topology (build succeed):

```
[connectivity]

slr=executer_1:SLR2
slr=scheduler_1:SLR2

sp=executer_1.in_pages_A:DDR[2]
sp=executer_1.in_pages_B:DDR[2]
sp=executer_1.layer_cache:DDR[2]
sp=executer_1.out_intersect:DDR[2]

[vivado] 

prop=run.impl_1.strategy=Performance_SpreadSLLs
```

Example topology (build failure):

```
[connectivity]

...

sp=executer_1.in_pages_A:DDR[0]
sp=executer_1.in_pages_B:DDR[0]
sp=executer_1.layer_cache:DDR[1]
sp=executer_1.out_intersect:DDR[1]

...

[vivado] 

prop=run.impl_1.strategy=Performance_SpreadSLLs

```


Example error message: 

```
[22:46:43] Phase 9 Verifying routed nets
[23:57:47] Phase 10 Depositing Routes
[00:20:09] Run vpl: Step impl: Failed
[00:20:14] Run vpl: FINISHED. Run Status: impl ERROR

===>The following messages were generated while processing /pub/scratch/wenqi/spatial-join-on-FPGA/FPGA_DFS/DFS_single_PE_block_read/_x.hw/link/vivado/vpl/prj/prj.runs/impl_1 :
ERROR: [VPL 18-5241] HPR Routing Violation - 12: Unlocked None-Partition net level0_i/level1/level1_i/ulp/ip_psr_aresetn_kernel_00_slr0/U0/peripheral_aresetn[0] does not belong to any PBLOCK. This is not expected for hierarchical Dynamic Function eXchange (DFX) flow.
ERROR: [VPL 60-773] In '/pub/scratch/wenqi/spatial-join-on-FPGA/FPGA_DFS/DFS_single_PE_block_read/_x.hw/link/vivado/vpl/vivado.log', caught Tcl error:  problem implementing dynamic region, impl_1: route_design ERROR, please look at the run log file '/pub/scratch/wenqi/spatial-join-on-FPGA/FPGA_DFS/DFS_single_PE_block_read/_x.hw/link/vivado/vpl/prj/prj.runs/impl_1/runme.log' for more information
ERROR: [VPL 60-704] Integration error, problem implementing dynamic region, impl_1: route_design ERROR, please look at the run log file '/pub/scratch/wenqi/spatial-join-on-FPGA/FPGA_DFS/DFS_single_PE_block_read/_x.hw/link/vivado/vpl/prj/prj.runs/impl_1/runme.log' for more information
```

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
