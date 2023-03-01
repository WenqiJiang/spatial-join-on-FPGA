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
    hls::stream<int>& s_join_finish_in,
    // output
    hls::stream<node_meta_t>& s_meta_A,
    hls::stream<node_meta_t>& s_meta_B,
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B,
    hls::stream<int>& s_join_finish_out
    ) {

    const int page_size_per_axi = page_bytes / 64; 

    while (true) {

        if (!axis_page_ID_pair_read_nodes.empty()) {

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

            s_meta_A.write(meta_A);
            s_meta_B.write(meta_B);
            
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
        } else if (!s_join_finish_in.empty()) {
            int end = s_join_finish_in.read();
            s_join_finish_out.write(end);
            break;
        }  
    }

}

void burst_buffer(
    // input
    hls::stream<result_t>& s_result_pair, 
    hls::stream<int>& s_join_finish_in, 

    // output
    hls::stream<int>& s_result_pair_burst_length, 
    hls::stream<result_t>& s_result_pair_burst, 
    hls::stream<int>& s_join_finish_out
) {
    // This buffer waits the result FIFO for N = 512 cycles, 
    //   such that we can write out the count of available contents in such interval,
    //   and the write PEs and layer cache manager can use it to infer burst write

    // burst_read_cycles must <= the output FIFO length
    const int burst_read_cycles = 512 - 1;

    while (true) {

        // note: the last signal will also be counted as burst length!
        // 'last' is guaranteed to be as the last element of a burst package
        int burst_length = 0;

        for (int i = 0; i < burst_read_cycles; i++) {
#pragma HLS pipeline II=1

            if (!s_result_pair.empty()) {

                result_t result = s_result_pair.read();
                s_result_pair_burst.write(result);
                burst_length++; 

                // if last, the join PE will not produce new result immediately,
                //    as it has to load the next pair of pages
                if (result.last) {
                    break;
                }
            }
        }

        if (burst_length > 0) {
            s_result_pair_burst_length.write(burst_length);
        }
        
        if (!s_join_finish_in.empty() && s_result_pair.empty()) {
            int break_signal = s_join_finish_in.read(); // must read to make dataflow work
            s_join_finish_out.write(break_signal);
            break;
        }
    }
}

void layer_cache_memory_controller(
    // input
    //   argument
    int root_id_A,
    int root_id_B,
    //   memory 
    ap_uint<64>* layer_cache,
    //   from join PE
    hls::stream<int>& s_intersect_count_directory, 
    hls::stream<int>& s_result_pair_directory_burst_length, 
    hls::stream<result_t>& s_result_pair_directory_burst, 
    //   from scheduler
    hls::stream<int>& axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& axis_layer_cache_read_addr, 
    hls::stream<int>& axis_layer_cache_write_addr, 
    hls::stream<int>& s_join_finish_in,
    // output
    //   to scheduler
    hls::stream<pair_t>& axis_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& axis_intersect_count_directory_scheduler, // for write request, return count
    hls::stream<int>& s_join_finish_out
) {

    // Initialization: write the pair (rootA, rootB) in layer cache 0
    pair_t root_pair;
    root_pair.id_A = root_id_A;
    root_pair.id_B = root_id_B;
    ap_uint<64> root_ap_uint_64 = pack_pair(root_pair);
    layer_cache[0] = root_ap_uint_64; 

    // Order:
    //   1. receive a signal from the scheduler: indicating whether it wants to write to / read from memory
    //   If it is a write:
    //      a. receive cache layer id from the scheduler
    //      b. receive the layer cache results from the join PE, write to memory
    //   If it is a read:
    //      a. receive the read layer id and the pointer, read it from memory
    //      b. write the pair back to the scheduler
    
    while (true) {

        if (!axis_read_write_control.empty()) {

            // 0 -> read from memory; 1 -> write to memory 
            int write = block_read<int>(axis_read_write_control);

            if (write) {

                int start_addr = block_read<int>(axis_layer_cache_write_addr);

                // 'last' will break this loop - we only target to write the results of a single page pair
                while (true) {

                    int burst_length = s_result_pair_directory_burst_length.read();

                    // 'last', if appeared in a burst, is guaranteed to be as the last element of a burst package
                    for (int i = 0; i < burst_length - 1; i++) {
#pragma HLS pipeline II=1

                        result_t result = s_result_pair_directory_burst.read();
                        ap_uint<64> result_ap_uint_64 = pack_pair(result.pair);
                        layer_cache[start_addr + i] = result_ap_uint_64;
                    }
                    // last iteration in the burst
                    // recompute start addr & count
                    result_t result = s_result_pair_directory_burst.read();
                    if (result.last) {
                        int count = s_intersect_count_directory.read(); 
                        axis_intersect_count_directory_scheduler.write(count);
                        break;
                    } else {
                        ap_uint<64> result_ap_uint_64 = pack_pair(result.pair);
                        layer_cache[start_addr + burst_length - 1] = result_ap_uint_64;
                        start_addr += burst_length;
                    }
                }


            } else { // read 
                int addr = block_read<int>(axis_layer_cache_read_addr);

                pair_t next_page_pair = unpack_pair(layer_cache[addr]);
                axis_page_pair_scheduler.write(next_page_pair);
            }
        } else if (!s_join_finish_in.empty()) {
            int end = s_join_finish_in.read();
            s_join_finish_out.write(end);
            break;
        } 
    }
}

void write_results(
    // input
    //   from join PE
    hls::stream<int>& s_intersect_count_leaf, // per page pair
    hls::stream<int>& s_result_pair_leaf_burst_length, 
    hls::stream<result_t>& s_result_pair_leaf_burst, 
    //   from the scheduler
    hls::stream<int>& s_join_finish_in,  // final end signal 
    // out
    //    out format: the first number writes total intersection count, 
    //                while the rest are intersect ID pairs
    ap_uint<64>* out_intersect) {

    ap_uint<64> total_intersect_count = 0;
    const int bias = 1; // the first number writes total intersection count

    while (true) {

        // 'last' will break this loop - we only target to write the results of a single page pair
        if (!s_result_pair_leaf_burst_length.empty()) {

            int burst_length = s_result_pair_leaf_burst_length.read();

            // 'last', if appeared in a burst, is guaranteed to be as the last element of a burst package
            for (int i = 0; i < burst_length - 1; i++) {
#pragma HLS pipeline II=1

                result_t result = s_result_pair_leaf_burst.read();
                ap_uint<64> result_ap_uint_64 = pack_pair(result.pair);
                out_intersect[total_intersect_count + i] = result_ap_uint_64;
            }
            // last iteration in the burst
            // recompute start addr & count
            result_t result = s_result_pair_leaf_burst.read();
            if (result.last) {
                total_intersect_count += burst_length - 1; // last does not count as result
                int tmp_count = s_intersect_count_leaf.read(); 
            } else {
                ap_uint<64> result_ap_uint_64 = pack_pair(result.pair);
                out_intersect[total_intersect_count + burst_length - 1] = result_ap_uint_64;
                total_intersect_count += burst_length;
            }
        }

        // the entire join is finished
        else if (!s_join_finish_in.empty() && s_result_pair_leaf_burst.empty()) {
            int end = s_join_finish_in.read();
            break;
        }
        
    }

    // write the number of intersection in the first address
    out_intersect[0] = total_intersect_count;
    
}
