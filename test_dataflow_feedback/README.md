# Test dataflow behavior when there are feedback communications


Suppose we have two PE A and B in a dataflow region connected by HLS stream. A writes a start signal to B (stream A2B); B receives the signal, fetches a value from DRAM channel B, and writes the value back to A (stream B2A); A then writes the value back to memory channel A. However, as simple as this, the generated bitstream will run into a deadlock. 

Directory: dataflow_feedback_case_1 (3 PEs)


      _____                    ______
     |     |                  |      |
     |     | -> stream A2B -> |      |
MemA-| PE A|                  | PE B |--MemB
     |     | <- stream B2A <- |      |
     |_____|                  |______|


Mario's feedback: using Vitis HLS cosim deadlock viewer https://docs.xilinx.com/r/2021.2-English/ug1399-vitis-hls/Cosim-Deadlock-Viewer

Result: once we use this cosim flow, the compilation filed... didn't even started the co-sim at all. If we use csim, the PEs execute the procedure sequentially rather than concurrently, which is wrong. 