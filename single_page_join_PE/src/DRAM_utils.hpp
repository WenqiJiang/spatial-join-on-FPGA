#pragma once

#include "types.hpp"

// send pairs of pages to the compute PE(s)
//   page_num_A * page_num_B pairs in total
// Note: the current design underutilizes the bandwidth for convenience
void input_loader(
    // input
    int page_entry_num, // number of entries per page
    int page_num_A, // number of pages for input A
    int page_num_B, // number of pages for input B
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B
    // output
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B
    ) {
    
    // number of 512-bit entries that a page consumes 
    const int addr_per_page = page_entry_num % N_OBJ_PER_AXI == 0?
        page_entry_num / N_OBJ_PER_AXI : page_entry_num / N_OBJ_PER_AXI + 1;

    for (int a = 0; a < page_num_A; a++) {
        for (int b = 0; b < page_num_B; b++) {
            int start_addr_A = addr_per_page * a;
            int start_addr_B = addr_per_page * b;

            for (int i = 0; i < addr_per_page; i++) {
#pragma HLS pipeline II=3
            // parse the input to three outputs
                ap_uint<512> reg_A = in_pages_A[start_addr_A + i];
                ap_uint<512> reg_B = in_pages_B[start_addr_B + i];

                for (int j = 0; j < N_OBJ_PER_AXI; j++) {

                    // page A
                    ap_uint<32> obj_id_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 1 - 1, j * OBJ_BITS + 32 * 0);
                    ap_uint<32> left_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 2 - 1, j * OBJ_BITS + 32 * 1);
                    ap_uint<32> right_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 3 - 1, j * OBJ_BITS + 32 * 2);
                    ap_uint<32> top_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 4 - 1, j * OBJ_BITS + 32 * 3);
                    ap_uint<32> bottom_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 5 - 1, j * OBJ_BITS + 32 * 4);

                    obj_t obj_A;
                    obj_A.obj_id_A = *((int*) (&obj_id_A_ap_uint_32));
                    obj_A.left_A = *((float*) (&left_A_ap_uint_32)); 
                    obj_A.right_A = *((float*) (&right_A_ap_uint_32)); 
                    obj_A.top_A = *((float*) (&top_A_ap_uint_32)); 
                    obj_A.bottom_A = *((float*) (&bottom_A_ap_uint_32)); 
                    s_page_A.write(obj_A);

                    // Page B
                    ap_uint<32> obj_id_B_Bp_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 1 - 1, j * OBJ_BITS + 32 * 0);
                    ap_uint<32> left_B_Bp_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 2 - 1, j * OBJ_BITS + 32 * 1);
                    ap_uint<32> right_B_Bp_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 3 - 1, j * OBJ_BITS + 32 * 2);
                    ap_uint<32> top_B_Bp_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 4 - 1, j * OBJ_BITS + 32 * 3);
                    ap_uint<32> bottom_B_Bp_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 5 - 1, j * OBJ_BITS + 32 * 4);

                    obj_t obj_B;
                    obj_B.obj_id_B = *((int*) (&obj_id_B_Bp_uint_32));
                    obj_B.left_B = *((float*) (&left_B_Bp_uint_32)); 
                    obj_B.right_B = *((float*) (&right_B_Bp_uint_32)); 
                    obj_B.top_B = *((float*) (&top_B_Bp_uint_32)); 
                    obj_B.bottom_B = *((float*) (&bottom_B_Bp_uint_32)); 
                    s_page_B.write(obj_B);
                }
            }
        }
    }
}