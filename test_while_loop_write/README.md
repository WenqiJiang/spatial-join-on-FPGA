# test_while_loop_writes

Test 1~7 proves that, either there's a while loop or a for loop with empty judgement, the write-to-memory burst cannot be inferred. Instead, we should define a protocol. 

Alternatively, we should develop a protocol, that indicates how many vectors are there in the data pipe, such that the write PE can use a for loop without empty check.

## manual burst buffer

**Even for consecutive writes, increasing burst length may not always help the performance! Setting a burst length of 32 should be enough. **

### modular_test_write_burst_buffer_default

For 100 million writes: 

1017.62 ms @140 MHz -> 1017.62 / 1000 / 1e8 / (1/140/1e6) = 1.42 cycle / write

### modular_test_write_burst_buffer_burst_length_32_write_outstranding_32

For 100 million writes: 

916.452  ms @140 MHz -> 916.452  / 1000 / 1e8 / (1/140/1e6) = 1.28 cycle / write


### modular_test_write_burst_buffer_burst_length_64

For 100 million writes: 

915.856 ms @140 MHz -> 915.856  / 1000 / 1e8 / (1/140/1e6) = 1.28 cycle / write

### modular_test_write_burst_buffer_burst_length_128

For 100 million writes: 

969.915 ms @140 MHz -> 969.915  / 1000 / 1e8 / (1/140/1e6) = 1.36 cycle / write

Even higher than burst length=64!

## test pure write bandwidth

The default write with the default AXI burst length (16) and default max write request on the fly (16), will not yield 1 cycle per write even the loop is a simple sequential write. 

**max_write_burst_length=128 will saturate performance (1.04 cycle per write)**

```
    for (long i = 0; i < page_pair_num * page_entry_num * page_entry_num; i++)  {
#pragma HLS pipeline II=1
        out_intersect[i] = result;
    }
```

project names: pure_write_...

### default num_write_outstanding=32 max_write_burst_length=32


For 100 million writes: 

937.501 ms @140 MHz -> 937.501 / 1000 / 1e8 / (1/140/1e6) = 1.31 cycle / write


### num_write_outstanding=32 max_write_burst_length=16


For 100 million writes: 

937.501 ms @140 MHz -> 937.501 / 1000 / 1e8 / (1/140/1e6) = 1.31 cycle / write

Exact the same as default -> max_write_burst_length doesn't help


### num_write_outstanding=32 max_write_burst_length=16

For 100 million writes: 

825.895 ms @140 MHz -> 825.895 / 1000 / 1e8 / (1/140/1e6) = 1.16 cycles per write. 

Same as max_write_burst_length=32: increasing burst size do help, increase max_write_burst_length doesn't!

### num_write_outstanding=32 max_write_burst_length=32

For 100 million writes: 

825.894 ms @140 MHz -> 825.894 / 1000 / 1e8 / (1/140/1e6) = 1.16 cycles per write. 

### num_write_outstanding=64 max_write_burst_length=64

For 100 million writes: 

770.091 ms @140 MHz -> 770.091 / 1000 / 1e8 / (1/140/1e6) = 1.08 cycles per write. 


### num_write_outstanding=16 max_write_burst_length=128

For 100 million writes: 

742.19 ms @140 MHz -> 742.19 / 1000 / 1e8 / (1/140/1e6) = 1.04 cycles per write. 

### optimized_write_results (default burst length)

1017.64 ms @ 140 MHz -> 1017.64 / 1000 / 1e8 / (1/140/1e6) = 1.42 cycles per write.

Conclusion: performance close to default for loop write (1.31 cycle / write). Thus the manual burst makes sense. 

### optimized_write_results_max_burst_length_128

964.085 ms @ 140 MHz -> 964.085 / 1000 / 1e8 / (1/140/1e6) = 1.35 cycles per write.

Conclusion: increase AXI burst length doesn't seem to help with the performance that much for the manual buffering case. 

