#pragma once

#include "types.hpp"

extern "C" {

// The scheduler keeps track of where the FPGA is working on during the tree traversal.
void scheduler(
    int* meta_info,
    // meta info: indicate when to send read/write request
    //  this must be consistent with the pages:
    //    for joins of data nodes -> send read node request; no need to send layer cache anything, but can do a read test 
    //    for joins of non-data pairs -> send to layer cache write request
    // structure: (3 int = 12 bytes)
    //    int page ID A;
    //    int page ID B;
    //    int data_join; // 1->is data join; 0->not data join
    // Input
    int iter_num, // number of iterations run on meta info
    int max_level_A,
    int max_level_B,
    //   from layer cache controller
    hls::stream<pair_t>& s_page_pair_scheduler,      // for read request, return pair
    hls::stream<int>& s_intersect_count_directory_scheduler, // for write request, return count
    // Output
    //   to layer cache controller
    hls::stream<int>& s_read_write_control, // 0 -> read from memory; 1 -> write to memory 
    hls::stream<int>& s_read_layer_id,      // layer l 
    hls::stream<int>& s_read_layer_pointer, // pair p in layer l
    hls::stream<int>& s_write_layer_id, 
    //   to node reading PE
    hls::stream<pair_t>& s_page_ID_pair_read_nodes,
    //   to the write results PE
    hls::stream<int>& s_join_finish         // the final termination signal 
) {

#pragma HLS INTERFACE m_axi port=meta_info offset=slave bundle=gmem0

// input streams
#pragma HLS INTERFACE axis port=s_page_pair_scheduler
#pragma HLS INTERFACE axis port=s_intersect_count_directory_scheduler

// output streams
#pragma HLS INTERFACE axis port=s_read_write_control
#pragma HLS INTERFACE axis port=s_read_layer_id
#pragma HLS INTERFACE axis port=s_read_layer_pointer
#pragma HLS INTERFACE axis port=s_write_layer_id
#pragma HLS INTERFACE axis port=s_page_ID_pair_read_nodes
#pragma HLS INTERFACE axis port=s_join_finish

    const int size_meta_unit = 3;
    int count = 0; 

    for (int i = 0; i < iter_num; i++) {
        int page_ID_A = meta_info[i * size_meta_unit + 0];
        int page_ID_B = meta_info[i * size_meta_unit + 1];
        int data_join = meta_info[i * size_meta_unit + 2];
        pair_t p;
        p.id_A = page_ID_A;
        p.id_B = page_ID_B;
        // send read node request; no need to send layer cache anything, but can do a read test
        if (data_join) {
            s_read_write_control.write(0);    
            s_read_layer_id.write(0);    
            s_read_layer_pointer.write(0);    
            pair_t result = s_page_pair_scheduler.read();
            count += result.id_A;
            s_page_ID_pair_read_nodes.write(p);
        }
        // for joins of non-data pairs -> send to layer cache write request
        else {
            s_read_write_control.write(1);    
            s_write_layer_id.write(0);
            int result = s_intersect_count_directory_scheduler.read();
            count += result;
        }
    }
    s_join_finish.write(count);

}

}