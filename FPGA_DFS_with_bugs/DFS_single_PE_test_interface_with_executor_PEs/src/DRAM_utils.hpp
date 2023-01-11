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
    const ap_uint<512>* in_pages_A,
    const ap_uint<512>* in_pages_B,
    hls::stream<pair_t>& axis_page_ID_pair_read_nodes,
    hls::stream<int>& s_join_finish_replicated,
    // output
    hls::stream<node_meta_t>& s_meta_A,
    hls::stream<node_meta_t>& s_meta_B,
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B
    ) {
    
    // read
    volatile pair_t reg_p = block_read<pair_t>(axis_page_ID_pair_read_nodes);
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
    hls::stream<result_t>& s_result_pair_directory,
    //   from scheduler
    hls::stream<int>& axis_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& axis_read_layer_id,      // layer l 
    hls::stream<int>& axis_read_layer_pointer, // pair p in layer l
    hls::stream<int>& axis_write_layer_id, 
    hls::stream<int>& s_join_finish_replicated,
    // output
    //   to scheduler
    hls::stream<pair_t>& axis_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& axis_intersect_count_directory_scheduler // for write request, return count
) {

    // write
    pair_t pair;
    axis_page_pair_scheduler.write(pair);
    int count;
    axis_intersect_count_directory_scheduler.write(count);

    // read
    volatile int read = block_read<int>(axis_read_write_control);
    volatile int layer_id = block_read<int>(axis_read_layer_id);
    volatile int layer_pointer = block_read<int>(axis_read_layer_pointer);

    volatile int write = block_read<int>(axis_write_layer_id);
}


// TODO: rewrite this, then write the scheduler
// slow in writing, no burst inferred
void write_results(
    // input
    //   from join PE
    hls::stream<int>& s_intersect_count_leaf, // per page pair
    hls::stream<result_t>& s_result_pair_leaf,
    //   from the scheduler
    hls::stream<int>& s_join_finish_replicated,  // final end signal 
    // out
    //    out format: the first number writes total intersection count, 
    //                while the rest are intersect ID pairs
    ap_uint<64>* out_intersect) {

}
