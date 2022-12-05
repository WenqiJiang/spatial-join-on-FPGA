# multi_kernel_test

This folder contains a bunch of behavior testing projects for the case of multi-kernel communication using AXI stream interface. 

Example Vitis multi-kernel programs: https://github.com/Xilinx/Vitis_Accel_Examples/blob/master/host/streaming_k2k_mm/src/host.cpp

Example free-running kernel: https://github.com/WenqiJiang/spatial-join-on-FPGA/commit/c673e20d0e00c3fe3773a5de0667dba0358ca777

Note that the free running kernel is stateless and cannot connect to memory interface, thus only suitable for simple tasks such as forwarding data.
## Key takeaways

### enable out of order execution queue on the host

Otherwise openCL will wait one task to finish before the next one, which is not going to work for concurrent kernel executions. 

```
    // Wenqi: enable out of order execution
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
```

### empty check at the beginning of a kernel is necessary

Check whether the stream is empty in the first iteration, otherwise the program can run into a deadlock. It also appears in EasyNet.

This also means that we'd better use the axis data type which contain an 'last' attribute for termination condition, rather having the termination signal along, because the first terminate signal itself is the end signal...

```
        if (first_iter) {
            while (s_A_to_B.empty()) {}
            first_iter = false;
        }
```

### better have a AXIS FIFO depth specified, e.g.,

```
# direction: master (interface which writes data) to slave (interface which reads data)
sc=PE_A_1.s_A_to_B:PE_B_1.s_A_to_B:64 # last argument is FIFO depth 
```

### can use the AXIS stream datatype, but this is not necessary

The benefit of using this is that it contains the ‘last’ entry for termination detection.

```
// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces-without-Side-Channels
#include <ap_axi_sdata.h>
typedef ap_axiu<32, 0, 0, 0> pkt;
hls::stream<pkt>& s_B_to_A
pkt p = s_B_to_A.read()
p.data
p.last
```

## Functionality of each folder

### multi_kernel_trial_1 

Simple AXI feedback operation between 2 PEs. 

No FIFO depth. 

No empty check.

Failed (stuck).


### multi_kernel_trial_12

Simple AXI feedback operation between 2 PEs. 

No FIFO depth. 

Has empty check.

Succeed. 


### multi_kernel_trial_3

Simple AXI feedback operation between 2 PEs. 

Using FIFO instead of AXIS as the top-level interface.

No FIFO depth. 

With empty check.

Compilation failed: using FIFO as top-level interface is not supported. 


### multi_kernel_trial_4

Simple AXI feedback operation between 2 PEs. 

Has FIFO depth of 64. 

Has empty check.

Succeed. 

### multi_kernel_trial_5

Simple AXI feedback operation between 2 PEs. 

Using AXIS data type.

Has FIFO depth. 

Has empty check.

Succeed. 

### multi_kernel_trial_6

Simple AXI feedback operation between 2 PEs. 

Has FIFO depth. 

No empty check.

Failed (stuck).


### multi_kernel_trial_7

Simple AXI feedback operation between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

No empty check.

Failed (stuck).

### multi_kernel_trial_8

Simple AXI feedback operation between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

Has empty check (first iteration).

Succeed

### multi_kernel_trial_single_direction_3

Single-direction AXI transfer between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

No empty check.

Succeed. 