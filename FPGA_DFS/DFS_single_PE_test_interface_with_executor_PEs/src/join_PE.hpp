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

}