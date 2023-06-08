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


### multi_kernel_trial_2

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

However, sometimes this message will ocur. The wierd thing is that, if I run this bitstream directly, it will complain about the message. If I run trial 5 (another successful version) and then trail 8, everthing works fine. 
 
```
[XRT] ERROR: unable to sync BO: Invalid argument
Duration (including memcpy out): 0.003 sec
First element of output: 0
TEST FINISHED (NO RESULT CHECK)
terminate called after throwing an instance of 'xrt_xocl::error'
  what():  event 5 never submitted
Aborted
```

Another wierd trace from summary.csv: The number of transfers is only 1, while the actual number of transfer should be 100.

```
Compute Unit Utilization
Device,Compute Unit,Kernel,Global Work Size,Local Work Size,Number Of Calls,Dataflow Execution,Max Overlapping Executions,Dataflow Acceleration,Total Time (ms),Minimum Time (ms),Average Time (ms),Maximum Time (ms),Clock Frequency (MHz),
xilinx_u250_gen3x16_xdma_shell_4_1-0,PE_A_1,PE_A,1:1:1,1:1:1,1,Yes,1,1.000000x,0.221693,0.221697,0.221693,0.221697,300,
xilinx_u250_gen3x16_xdma_shell_4_1-0,PE_B_1,PE_B,1:1:1,1:1:1,1,Yes,1,1.000000x,0.0305167,0.03052,0.0305167,0.03052,300,

Data Transfer: Streams
Device,Master Port,Master Kernel Arguments,Slave Port,Slave Kernel Arguments,Number Of Transfers,Transfer Rate (MB/s),Average Size (KB),Link Utilization (%),Link Starve (%),Link Stall (%),
xilinx_u250_gen3x16_xdma_shell_4_1-0,PE_A_1/s_A_to_B,s_A_to_B,PIPE,PIPE,1,0.0193374,0.4,0.00161145,99.9984,0,
xilinx_u250_gen3x16_xdma_shell_4_1-0,PIPE,PIPE,PE_A_1/s_B_to_A,s_B_to_A,1,0.0195118,0.4,0.00162598,99.9984,0,
xilinx_u250_gen3x16_xdma_shell_4_1-0,PIPE,PIPE,PE_B_1/s_A_to_B,s_A_to_B,1,0.0193219,0.4,0.00161016,99.9984,0,
xilinx_u250_gen3x16_xdma_shell_4_1-0,PE_B_1/s_B_to_A,s_B_to_A,PIPE,PIPE,1,0.019496,0.4,0.00162467,99.9984,0,
```

### multi_kernel_trial_9

Simple AXI feedback operation between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

Passing a start signal (a different signal to data signal) to each other, rather than checking data empty.

Failed -> (compare to experiment 8) this means that the PE itself starts does not count. The first data packet must arrive...

### multi_kernel_trial_9.5

Same as 9, except that we use the start signal in the loop to make sure the variable is not optimized away

Failed (stuck). -> volatile and dependency isn't an issue. 
### multi_kernel_trial_10

Simple AXI feedback operation between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

Passing start signals for all the FIFOs at the very beginning, rather than checking data empty.

### multi_kernel_trial_10.5

Same as 10, except that we use the start signal in the loop to make sure the variable is not optimized away

Failed (stuck). -> volatile and dependency isn't an issue. 

### multi_kernel_trial_11

Simple AXI feedback operation between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

For each read operation, check data empty.

Succeed! 

Performance: 290 ms for 1M iterations. Each iteration has 2 DRAM access (one read and one write), ~200 ns -> 200 ns * 1M ~= 200 ms. We need another program to test the while loop check consumption itself.


### multi_kernel_trial_12

Simple AXI feedback operation between 2 PEs. Only 1 read and 1 write operation in total, for performance test purpose.

With a loop.

Using AXIS data type.

Has FIFO depth. 

For each read operation, check data empty.

Succeed.

Performance: 36.6693 ms for 1 million iteration @300 MHz -> ~12 cycles per round. 1 Round = PE A write + PE B block read + PE B write + PE A block read. 4 operations in 12 cycles in reasonable, especially when we don't know AXIS latency can reach 1 cycle. 

### multi_kernel_trial_13

Similar structure to the spatial DB structure. 

Kernel PE A is standalone (like the scheduler).

Kernel PE B contains 3 sub-PEs (like the executor), each of them forward data to 

With a loop.

No AXIS data type.

Has FIFO depth. 

For each read operation, check data empty.

Succeed.

But somehow the summary.csv does not show performance numbers. In C++ measurement, it takes 45 ms for 1 million iteration (~15 cycles per round trip), quite reasonable given that we have 3 PEs in kernel PE B right now. 

### multi_kernel_trial_single_direction_3

Single-direction AXI transfer between 2 PEs. 

With a loop.

Using AXIS data type.

Has FIFO depth. 

No empty check.

Succeed. 

### multi_kernel_different_speed_trial_1

Given that the write FIFO writes data much after than the consumer (without checking whether the axis FIFO is full), check whether the program will stuck.

Results: succeed -> so only read needs empty check


### multi_kernel_different_speed_trial_2

Same as 1, but with full FIFO check.

Succeed.

### Compare Random Read Performance

Simulate the read behavior in the spatial join accelerator, in which the read addr and length are passed into the executor. 

300 MHz

An official note about latency in reading: https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Options-for-Controlling-AXI4-Burst-Behavior

"Getting through the adapter will cost a few cycles of latency, typically 5 to 7 cycles. Then, the request goes to the AXI interconnect that routes the kernel’s request to the MIG and then eventually to the DDR. Getting through the interconnect is expensive in latency and can take around 30 cycles. Finally, getting to the DDR and back can cost anywhere from 9 to 14 cycles"

#### multi_kernel_read_from_mem_1 

312.003 ms for 1M reads (read length = 8)

II = 0.312 * (300 * 1e6) / 1e6 = 93.6 cycles per read 

#### multi_kernel_read_from_mem_2 

300.001 ms  for 1M reads (read length = 8)

II = 0.3 * (300 * 1e6) / 1e6 = 90 cycles per read -> slightly better, maybe due to the burst length specified on the AXI interface 

#### multi_kernel_read_from_mem_3

280.844 ms for 1M reads (read length = 8)

II = 0.28 * (300 * 1e6) / 1e6 = 84 cycles per read -> 250 ns of latency seem to be the best U250 can achieve

#### multi_kernel_read_from_mem_3_latency_0

280.844 ms -> this is exactly the same as the default latency

NOTE: latency = 0 means use default, which = 64

#### multi_kernel_read_from_mem_3_latency_1

83.1312 ms -> extremely fast! -> up to 12 million reads per sec.

II = 0.083 * (300 * 1e6) / 1e6 = 25 cycles (given lenght = 8, the pure latency is 25 - 8 + 1 = 18 cycles)

the delta latency is 63 cycles

(84-63)/84 * 280 = 70 ms -> close to our expectation.

#### multi_kernel_read_from_mem_3_latency_32

180.206 ms -> much faster than the default latency

II = 0.18 * (300 * 1e6) / 1e6 = 60 cycles (given lenght = 8, the pure latency is 60 - 8 + 1 = 53 cycles)

the delta latency is 32 cycles

(84-32)/84 * 280 = 173 ms -> consistent to our expectation


#### multi_kernel_read_from_mem_4

* two input mem channels
* constant read size set as a parameter
* the read request is only issued once a while (gap cycles set as an input parameter) -> so burst read may not always be possible
* latency = 1
* run at 200 MHz

1 million reads, length = 8, send interval = 16 cycles (probably no burst reads) : 130.94 ms -> ~130 ns per new read

1 million reads, length = 8, send interval = 32 cycles (probably no burst reads) : 175.003 ms -> 175 ns per new read

1 million reads, length = 8, send interval = 64 cycles (probably no burst reads) : 335.003 ms -> 335 ns per new read


#### multi_kernel_write_to_mem_1

adjusted from multi_kernel_read_from_mem_3_latency_1.

* latency = 1
* 200 MHz

1 million writes of length 8 -> 
PE_A 116.329 ms -> ~8 million writes per sec
PE_B 385.598 ms -> ???? why so large gap??? 

I see: the time includes part of memcpy, when PE_A is started, it's waiting for memcpy to finish thus PE B can start. 

But these writes are merged into larger writes probably