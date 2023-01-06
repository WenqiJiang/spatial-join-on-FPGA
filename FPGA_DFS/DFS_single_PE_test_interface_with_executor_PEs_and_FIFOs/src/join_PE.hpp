#pragma once

#include "types.hpp"

// join between node pairs
// if both the nodes are leaf, write to the results
// if at least on of the nodes is directory, write to layer cache 
void join_page(
    // input
    hls::stream<node_meta_t>& s_meta_A,
    hls::stream<node_meta_t>& s_meta_B,
    hls::stream<obj_t>& s_page_A,
    hls::stream<obj_t>& s_page_B,
    hls::stream<int>& s_join_finish_replicated,
    // output
    //   for directory nodes: 
    hls::stream<int>& s_intersect_count_directory, // per page pair
    hls::stream<result_t>& s_result_pair_directory,
    //   for leaf nodes: 
    hls::stream<int>& s_intersect_count_leaf, // per page pair
    hls::stream<result_t>& s_result_pair_leaf
    ) {

    // write FIFO 
    int reg_int;
    result_t reg_result_t;

    s_intersect_count_directory.write(reg_int);
    s_result_pair_directory.write(reg_result_t);
    s_intersect_count_leaf.write(reg_int);
    s_result_pair_leaf.write(reg_result_t);

    // read FIFO

    volatile node_meta_t reg_A = block_read<node_meta_t>(s_meta_A);
    volatile node_meta_t reg_B = block_read<node_meta_t>(s_meta_B);
    volatile obj_t reg_C = block_read<obj_t>(s_page_A);
    volatile obj_t reg_D = block_read<obj_t>(s_page_B);
    volatile int reg_E = block_read<int>(s_join_finish_replicated);

}