#pragma once

#include "types.hpp"

// Given an input 64-byte representation, parse it as node meta data type
inline node_meta_t parse_meta_data(ap_uint<512> in_uint512) {

    node_meta_t meta_data;

    ap_uint<32> reg_is_leaf_uint32 = in_uint512.range(32 * 0 + 31, 32 * 0);
    meta_data.is_leaf = *((int*) (&reg_is_leaf_uint32));

    ap_uint<32> reg_count_uint32 = in_uint512.range(32 * 1 + 31, 32 * 1);
    meta_data.count = *((int*) (&reg_count_uint32));

    ap_uint<32> reg_obj_id_uint32 = in_uint512.range(32 * 2 + 31, 32 * 2);
    meta_data.obj.id = *((int*) (&reg_obj_id_uint32));

    ap_uint<32> reg_obj_low0_uint32 = in_uint512.range(32 * 3 + 31, 32 * 3);
    meta_data.obj.low0 = *((float*) (&reg_obj_low0_uint32));

    ap_uint<32> reg_obj_high0_uint32 = in_uint512.range(32 * 4 + 31, 32 * 4);
    meta_data.obj.high0 = *((float*) (&reg_obj_high0_uint32));

    ap_uint<32> reg_obj_low1_uint32 = in_uint512.range(32 * 5 + 31, 32 * 5);
    meta_data.obj.low1 = *((float*) (&reg_obj_low1_uint32));

    ap_uint<32> reg_obj_high1_uint32 = in_uint512.range(32 * 6 + 31, 32 * 6);
    meta_data.obj.high1 = *((float*) (&reg_obj_high1_uint32));

    return meta_data;
}

// send pairs of pages to the compute PE(s)
//   page_num_A * page_num_B pairs in total
// Note: the current design underutilizes the bandwidth for convenience
void input_loader(
    // input
    int page_num_A, // number of pages for input A
    int page_num_B, // number of pages for input B
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    // output
    hls::stream<node_meta_t>& s_meta_A,
    hls::stream<node_meta_t>& s_meta_B,
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B
    ) {
    

    for (int a = 0; a < page_num_A; a++) {
        for (int b = 0; b < page_num_B; b++) {
            int start_addr_A = PAGE_SIZE_PER_AXI * a;
            int start_addr_B = PAGE_SIZE_PER_AXI * b;

            // read meta data to get the page
            node_meta_t meta_A = parse_meta_data(in_pages_A[start_addr_A]);
            node_meta_t meta_B = parse_meta_data(in_pages_B[start_addr_B]);

            s_meta_A.write(meta_A);
            s_meta_B.write(meta_B);

            int max_page_entries = meta_A.count >= meta_B.count? meta_A.count : meta_B.count;
            // number of 512-bit entries that a page contains 
            int addr_per_page = max_page_entries % N_OBJ_PER_AXI == 0?
                max_page_entries / N_OBJ_PER_AXI : max_page_entries / N_OBJ_PER_AXI + 1;

            start_addr_A++;
            start_addr_B++;

            for (int i = 0; i < addr_per_page; i++) {
#pragma HLS pipeline // II=3 // needs N_OBJ_PER_AXI cycles
                // parse the input to three outputs
                ap_uint<512> reg_A = in_pages_A[start_addr_A + i];
                ap_uint<512> reg_B = in_pages_B[start_addr_B + i];

                for (int j = 0; j < N_OBJ_PER_AXI; j++) {

                    // page A
                    ap_uint<32> id_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 1 - 1, j * OBJ_BITS + 32 * 0);
                    ap_uint<32> low0_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 2 - 1, j * OBJ_BITS + 32 * 1);
                    ap_uint<32> high0_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 3 - 1, j * OBJ_BITS + 32 * 2);
                    ap_uint<32> low1_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 4 - 1, j * OBJ_BITS + 32 * 3);
                    ap_uint<32> high1_A_ap_uint_32 = reg_A.range(
                      j * OBJ_BITS + 32 * 5 - 1, j * OBJ_BITS + 32 * 4);

                    obj_t obj_A;
                    obj_A.id = *((int*) (&id_A_ap_uint_32));
                    obj_A.low0 = *((float*) (&low0_A_ap_uint_32)); 
                    obj_A.high0 = *((float*) (&high0_A_ap_uint_32)); 
                    obj_A.low1 = *((float*) (&low1_A_ap_uint_32)); 
                    obj_A.high1 = *((float*) (&high1_A_ap_uint_32)); 
                    if (i * N_OBJ_PER_AXI + j < meta_A.count) {
                      s_page_A.write(obj_A);
                    }

                    // Page B
                    ap_uint<32> id_B_ap_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 1 - 1, j * OBJ_BITS + 32 * 0);
                    ap_uint<32> low0_B_ap_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 2 - 1, j * OBJ_BITS + 32 * 1);
                    ap_uint<32> high0_B_ap_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 3 - 1, j * OBJ_BITS + 32 * 2);
                    ap_uint<32> low1_B_ap_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 4 - 1, j * OBJ_BITS + 32 * 3);
                    ap_uint<32> high1_B_ap_uint_32 = reg_B.range(
                      j * OBJ_BITS + 32 * 5 - 1, j * OBJ_BITS + 32 * 4);

                    obj_t obj_B;
                    obj_B.id = *((int*) (&id_B_ap_uint_32));
                    obj_B.low0 = *((float*) (&low0_B_ap_uint_32)); 
                    obj_B.high0 = *((float*) (&high0_B_ap_uint_32)); 
                    obj_B.low1 = *((float*) (&low1_B_ap_uint_32)); 
                    obj_B.high1 = *((float*) (&high1_B_ap_uint_32)); 
                    if (i * N_OBJ_PER_AXI + j < meta_B.count) {
                      s_page_B.write(obj_B);
                    }
                }
            }
        }
    }
}


// slow in writing, no burst inferred
void write_results(
    // input
    int page_pair_num, // number of page pairs to join, e.g., page_num_A * page_num_B
    hls::stream<ap_uint<64> >& s_intersect_count, // per page pair
    hls::stream<int>& s_join_finish,  // per page pair
    hls::stream<result_t>& s_result_pair, 
    // out format: the first number writes total intersection count, 
    //   while the rest are intersect ID pairs
    ap_uint<64>* out_intersect) {

    ap_uint<64> total_intersect_count = 0;
    const int bias = 1; // the first number writes total intersection count, 


    for (int i = 0; i < page_pair_num; i++) {

        bool reach_end = true; 
        for (int i = 0; i < MAX_PAGE_ENTRIES * MAX_PAGE_ENTRIES; i++) {
#pragma HLS pipeline II=1
            if (!s_join_finish.empty() && s_result_pair.empty()) {
                int break_signal = s_join_finish.read(); // must read to make dataflow work
                total_intersect_count += s_intersect_count.read();
                reach_end = false;
                break;
            }
            result_t result = s_result_pair.read();
            ap_uint<64> result_ap_uint_64;
            result_ap_uint_64.range(31, 0) = result.id_A;
            result_ap_uint_64.range(63, 32) = result.id_B;
            out_intersect[total_intersect_count + i + bias] = result_ap_uint_64;
        }
        if (reach_end) { // maximum results, haven't read the count yet 
            int break_signal = s_join_finish.read(); // must read to make dataflow work
            total_intersect_count += s_intersect_count.read();
        }
    }

    // write the number of intersection in the first address
    out_intersect[0] = total_intersect_count;
}