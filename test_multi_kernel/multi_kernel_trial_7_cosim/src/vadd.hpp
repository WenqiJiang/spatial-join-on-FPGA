#include "constants.hpp"
#include "DRAM_utils.hpp"
#include "join_PE.hpp"
#include "scheduler.hpp"
#include "types.hpp"

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces-without-Side-Channels
typedef ap_axiu<32, 0, 0, 0> pkt;

extern "C" {
    
void PE_A(
    // in
    int iter,
    hls::stream<pkt>& s_B_to_A,
    // out
    hls::stream<pkt>& s_A_to_B,
    int* output);

void PE_B(
    // in
    int iter,
    int* input,
    hls::stream<pkt>& s_A_to_B,
    // out
    hls::stream<pkt>& s_B_to_A);

}