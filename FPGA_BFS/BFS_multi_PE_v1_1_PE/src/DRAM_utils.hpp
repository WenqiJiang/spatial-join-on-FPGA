#pragma once

#include "types.hpp"
#include "utils.hpp"

// Given an input 64-byte representation, parse it as node meta data type
node_meta_t parse_meta_data(ap_uint<512> in_uint512) {
#pragma HLS inline

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

// Input: the read request (pair of page IDs) from the scheduler
// Output: the meta data and entries of the nodes of the pair of nodes
// Note: the current design underutilizes the bandwidth for convenience
void read_nodes(
    // input
    const int page_bytes, 
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    hls::stream<pair_t>& axis_page_ID_pair_read_nodes,
    hls::stream<int>& axis_join_PE_ID,
    hls::stream<int>& s_join_finish_in,
    // output
    hls::stream<node_meta_t> (&s_meta_A)[N_JOIN_PE],
    hls::stream<node_meta_t> (&s_meta_B)[N_JOIN_PE],
    hls::stream<obj_t> (&s_page_A)[N_JOIN_PE],
    hls::stream<obj_t> (&s_page_B)[N_JOIN_PE],
    hls::stream<int>& s_join_finish_out
    ) {

    const int page_size_per_axi = page_bytes / 64; 

    while (true) {

        if (!axis_page_ID_pair_read_nodes.empty()) {

            int join_PE_ID = block_read<int>(axis_join_PE_ID);

            pair_t page_ID_pair = block_read<pair_t>(axis_page_ID_pair_read_nodes);
            int page_ID_A = page_ID_pair.id_A;
            int page_ID_B = page_ID_pair.id_B;

            int start_addr_A = page_size_per_axi * page_ID_A;
            int start_addr_B = page_size_per_axi * page_ID_B;

            // read meta data to get the page
            node_meta_t meta_A = parse_meta_data(in_pages_A[start_addr_A]);
            node_meta_t meta_B = parse_meta_data(in_pages_B[start_addr_B]);
            start_addr_A++;
            start_addr_B++;

            s_meta_A[join_PE_ID].write(meta_A);
            s_meta_B[join_PE_ID].write(meta_B);
            
            int max_page_entries = meta_A.count >= meta_B.count? meta_A.count : meta_B.count;
            // number of 512-bit entries that a page contains 
            int addr_per_page = max_page_entries % N_OBJ_PER_AXI == 0?
                max_page_entries / N_OBJ_PER_AXI : max_page_entries / N_OBJ_PER_AXI + 1;

            // the 64-byte header is already counted by ++
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
                        s_page_A[join_PE_ID].write(obj_A);
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
                        s_page_B[join_PE_ID].write(obj_B);
                    }
                }
            }
        } else if (!s_join_finish_in.empty()) {
            int end = s_join_finish_in.read();
            s_join_finish_out.write(end);
            break;
        }  
    }

}


void layer_cache_memory_controller(
    // input
    //   argument
    int max_level_A,
    int max_level_B,
    int root_id_A,
    int root_id_B,
    //   memory 
    ap_uint<64>* layer_cache,
    //   from join PE
    hls::stream<int> (&s_intersect_count_directory)[N_JOIN_PE], 
    hls::stream<result_t> (&s_result_pair_directory)[N_JOIN_PE],
    //   from scheduler
    hls::stream<int>& axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& axis_layer_cache_read_addr, 
    hls::stream<int>& axis_layer_cache_write_addr, 
    hls::stream<int>& axis_num_layer_pairs, // number of pairs to join in this layer
    hls::stream<int>& s_join_finish_in,
    // output
    //   to scheduler
    hls::stream<pair_t>& axis_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& axis_intersect_count_directory_scheduler, // for write request, return count
    hls::stream<int>& s_join_finish_out
) {

    int max_level = max_level_A > max_level_B ? max_level_A: max_level_B;
    
    // Initialization: write the pair (rootA, rootB) in layer cache 0
    pair_t root_pair;
    root_pair.id_A = root_id_A;
    root_pair.id_B = root_id_B;
    ap_uint<64> root_ap_uint_64 = pack_pair(root_pair);
    layer_cache[0] = root_ap_uint_64; 


    // Order:
    //   1. For each layer, read the write control signal, the write address, and the total
    //        number of pairs in the layer
    //   2. Loop over the join PEs to collect results; check read cache request regularly; 
    //        end the loop if the number of last signal by join PE == number of pairs in the layer

    // non-leaf join, write to the layer cache 
    for (int current_level = 1; current_level < max_level + 1; current_level++) {

        // number of pairs to process in the layer
        int num_layer_pairs = block_read<int>(axis_num_layer_pairs);

        // write signal and address
        int write_signal = block_read<int>(axis_read_write_control); // should be 1 -> write
        int write_start_addr = block_read<int>(axis_layer_cache_write_addr);
        int write_count = 0;
        int last_count = 0; // number of page joins finished
        bool break_while = false;

        while (true) {

            if (break_while) {
                break;
            }

            // read signal arrives
            else if (!axis_read_write_control.empty()) { 
                int read_signal = block_read<int>(axis_read_write_control); // should be 0 -> read
                int addr = block_read<int>(axis_layer_cache_read_addr);
                pair_t next_page_pair = unpack_pair(layer_cache[addr]);
                axis_page_pair_scheduler.write(next_page_pair);
            } 

            // write to memory by loading content from join PEs
            else { 

                // chech all PEs in a round-robin manner
                for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {

                    if (!s_result_pair_directory[PE_id].empty()) {

                        result_t result = s_result_pair_directory[PE_id].read();

                        if (result.last) {
                            int tmp_count = s_intersect_count_directory[PE_id].read(); 
                            last_count++;
                            if (last_count == num_layer_pairs) {
                                break_while = true; 
                            }
                        } else {
                            ap_uint<64> result_ap_uint_64 = pack_pair(result.pair);
                            layer_cache[write_start_addr + write_count] = result_ap_uint_64;
                            write_count++;
                        }
                    }
                }
            }
        }

        // write the total number of pairs in this layer
        if (current_level < max_level) {    
            axis_intersect_count_directory_scheduler.write(write_count);
        }
    }

    int end = s_join_finish_in.read();
    s_join_finish_out.write(end);
}


// TODO: rewrite this, then write the scheduler
// slow in writing, no burst inferred
void write_results(
    // input
    //   from join PE
    hls::stream<int> (&s_intersect_count_leaf)[N_JOIN_PE], // per page pair
    hls::stream<result_t> (&s_result_pair_leaf)[N_JOIN_PE],
    //   from the scheduler
    hls::stream<int>& s_join_finish_in,  // final end signal 
    // out
    //    out format: the first number writes total intersection count, 
    //                while the rest are intersect ID pairs
    ap_uint<64>* out_intersect) {

    ap_uint<64> total_intersect_count = 0; 
    const int bias = 1; // the first number writes total intersection count

    while (true) {

        // whether any PE has results in this round, used for finish detection
        bool has_content = false; 

        // loop over join PEs to read results
        for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {

            // for each pair of page join, there'll be a last signal sent
            if (!s_result_pair_leaf[PE_id].empty()) {

                has_content = true; 
                result_t result = s_result_pair_leaf[PE_id].read();
                if (result.last) {
                    int tmp_count = s_intersect_count_leaf[PE_id].read(); 
                } else {
                    ap_uint<64> result_ap_uint_64 = pack_pair(result.pair);
                    out_intersect[total_intersect_count + bias] = result_ap_uint_64;
                    total_intersect_count++;
                }
            }
        }

        // check whether the entire join is finished
        if (!has_content && !s_join_finish_in.empty()) {

            bool has_content_recheck = false; // whether any PE has results in this round

            // loop over the FIFOs to double check if there's any content
            for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {
                if (!s_result_pair_leaf[PE_id].empty()) {
                    has_content_recheck = true; 
                    break;
                }
            }
            if (!has_content_recheck) {
                int end = s_join_finish_in.read();
                break;
            }
        }
    }

    // write the number of intersection in the first address
    out_intersect[0] = total_intersect_count;
}
