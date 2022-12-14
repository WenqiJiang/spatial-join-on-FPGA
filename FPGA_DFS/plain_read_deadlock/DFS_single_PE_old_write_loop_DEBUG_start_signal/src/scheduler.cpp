#pragma once

#include "types.hpp"

extern "C" {

// The scheduler keeps track of where the FPGA is working on during the tree traversal.
void scheduler(
    // Start signals
    hls::stream<int>& s_scheduler_start,
    hls::stream<int>& s_executer_start,
    // Input
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

#ifdef DEBUG_scheduler
    ////// debug starts

    s_scheduler_start.write(1);
    while (s_executer_start.empty()) {}
    // wait for 100 cycles for other signals to cone
    volatile int idle_count = 0;
    for (int i = 0; i < 100; i++) {idle_count++;}
    int start = s_executer_start.read();    

    int max_level = max_level_A > max_level_B ? max_level_A: max_level_B;

    // send read request, and get a pair
    s_read_write_control.write(0);
    s_read_layer_id.write(0);
    s_read_layer_pointer.write(0);
    pair_t pair = s_page_pair_scheduler.read();
    // also send to read nodes
    s_page_ID_pair_read_nodes.write(pair);

    // send write requests
    s_read_write_control.write(1);
    s_write_layer_id.write(0);
    int reg = s_intersect_count_directory_scheduler.read();
        
    s_join_finish.write(reg);

    ////// debug ends
#else

    int max_level = max_level_A > max_level_B ? max_level_A: max_level_B;
    int num_pairs_per_level[MAX_TREE_LEVEL] = {0};
    int current_pointer_per_level[MAX_TREE_LEVEL] = {0};

    // starting from fetching the pair of level 0, i.e., (root_A, root_B)
    num_pairs_per_level[0] = 1;
    int current_level = 1; 
    bool up_to_down = true;

    while (true) {

        // end condition: the entire join is finished
        if (current_pointer_per_level[0] == num_pairs_per_level[0]) {
            s_join_finish.write(1);
            break;
        } else {

            // up to down -> call join nodes  
            if (up_to_down) {
                
                // get the node pair to join, and send it to the read node PE
                s_read_write_control.write(0); // 0 -> read
                s_read_layer_id.write(current_level - 1);      // layer l 
                s_read_layer_pointer.write(current_pointer_per_level[current_level - 1]); // pair p in layer l
                pair_t node_pairs = s_page_pair_scheduler.read();

                // the join PE will automatically compute the results of the load nodes
                s_page_ID_pair_read_nodes.write(node_pairs);   
                s_write_layer_id.write(current_level);
                
                // move levels and pointers
                if (current_level == max_level) {
                    current_level--;
                    current_pointer_per_level[current_level]++;
                    up_to_down = false;
                } else {
                    // get result count from the scheduler 
                    int count = s_intersect_count_directory_scheduler.read();
                    
                    // empty: return to the higher level
                    if (count == 0) { 
                        current_level--;
                        current_pointer_per_level[current_level]++;
                        up_to_down = false;
                    } else { // has content: start in the lower level
                        num_pairs_per_level[current_level] = count;
                        current_pointer_per_level[current_level] = 0;
                        current_level++;
                        up_to_down = true;
                    }
                }
            } 
            // down to up -> check the pointer status of the upper layer and proceed
            else {
                // if at the end of the current level cache -> move to the upper level, else continue join
                if (current_pointer_per_level[current_level] == num_pairs_per_level[current_level]) {
                    current_level--;
                    current_pointer_per_level[current_level]++;
                    up_to_down = false;
                }
                else {
                    current_level++;
                    up_to_down = true;
                }
            }
        }
    }
#endif
}

}