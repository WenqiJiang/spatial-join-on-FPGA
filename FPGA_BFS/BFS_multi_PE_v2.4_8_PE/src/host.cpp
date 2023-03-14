#include <stdint.h>
#include <chrono>
#include <regex> 

#include "host.hpp"

#include "constants.hpp"
// Wenqi: seems 2022.1 somehow does not support linking ap_uint.h to host?
// #include "ap_uint.h"

using namespace std;

// boost::filesystem does not compile well, so implement this myself
string dir_concat(string dir1, string dir2) {
    if (dir1.back() != '/') {
        dir1 += '/';
    }
    return dir1 + dir2;
}

void wait_for_enter(const std::string &msg) {
    std::cout << msg << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main(int argc, char** argv)
{
    cl_int err;
    // Allocate Memory in Host Memory
    // When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the hood user ptr 
    // is used if it is properly aligned. when not aligned, runtime had no choice but to create
    // its own host side buffer. So it is recommended to use this allocator if user wish to
    // create buffer using CL_MEM_USE_HOST_PTR to align user buffer to page boundary. It will 
    // ensure that user buffer is used when user create Buffer/Mem object with CL_MEM_USE_HOST_PTR 

    int max_level_A;
    int max_level_B;
    int root_id_A = 0;
    int root_id_B = 0;
    int page_bytes = 4096;

    string tree_parent_dir = "/mnt/scratch/wenqi/spatial-join-on-FPGA/tree_bin";
    // string tree_fname = "sample_tree_level_1_self_join_156.bin";
    // string tree_fname = "sample_tree_level_2_self_join_2090.bin";
    // string tree_fname = "sample_tree_level_3_self_join_19246.bin";
    // string tree_fname = "sample_tree_level_4_self_join_235112.bin";
    string tree_fname = "sample_tree_level_5_self_join_5182308.bin";
    string tree_bin_dir = dir_concat(tree_parent_dir, tree_fname);

    // regex in c++: https://www.softwaretestinghelp.com/regex-in-cpp/
    regex reg_level("level_[0-9]+");
    regex reg_intersects("self_join_[0-9]+");

    smatch level_match; 
    smatch intersects_match; 
   
    // regex_search that searches pattern regexp in the string mystr  
    regex_search(tree_fname, level_match, reg_level);
    regex_search(tree_fname, intersects_match, reg_intersects);

    string level_str; // = level_match.begin().str().substr(6);
    string intersect_str; // = intersects_match.begin().str().substr(10);
    for (auto x : level_match) level_str = x.str().substr(6);
    for (auto x : intersects_match) intersect_str = x.str().substr(10);

    max_level_A = stol(level_str);
    max_level_B = stol(level_str);

    long correct_intersect = stol(intersect_str);

    cout << "Parsed tree level A : " << max_level_A << endl;
    cout << "Parsed tree level B : " << max_level_B << endl;
    cout << "Parsed intersect counts : " << correct_intersect << endl;

    // get data size from disk 
    ifstream tree_fstream(
        tree_bin_dir, ios::in | ios::binary);
    tree_fstream.seekg(0, tree_fstream.end);
    int64_t tree_bytes = tree_fstream.tellg();
    tree_fstream.seekg(0, tree_fstream.beg);
    cout << "Tree bytes: " << tree_bytes << endl;

    // allocate memory 
    cout << "Allocating memory...\n";
    size_t bytes_page_A = tree_bytes;
    size_t bytes_page_B = tree_bytes;
    vector<int ,aligned_allocator<int>> in_pages_A(bytes_page_A / sizeof(int), 0);
    vector<int ,aligned_allocator<int>> in_pages_B(bytes_page_B / sizeof(int), 0);

    // size_t out_bytes = 10 * 1024 * 1024;
    size_t layer_cache_bytes = 1 * size_t(1000) * size_t(1000) * size_t(1000); // no more than 16 GB
    cout << "layer_cache_bytes: " << layer_cache_bytes << endl;
    vector<int64_t ,aligned_allocator<int64_t>> layer_cache(layer_cache_bytes / sizeof(int64_t), 0);

    // size_t out_bytes = 10 * 1024 * 1024;
    size_t out_bytes = 1 * size_t(1000) * size_t(1000) * size_t(1000); // no more than 16 GB
    cout << "out_bytes: " << out_bytes << endl;
    vector<int64_t ,aligned_allocator<int64_t>> out(out_bytes / sizeof(int64_t), 0);

    // load data from disk
    tree_fstream.read((char*) in_pages_A.data(), tree_bytes);
    if (!tree_fstream) {
            cout << "error: only " << tree_fstream.gcount() << " could be read";
        exit(1);
    }
    memcpy(in_pages_B.data(), in_pages_A.data(), tree_bytes);

    cout << "Root A info: " << endl;
    cout << "  is leaf: " << in_pages_A[0] << endl;
    cout << "  count: " << in_pages_A[1] << endl;

    cout << "Root B info: " << endl;
    cout << "  is leaf: " << in_pages_B[0] << endl;
    cout << "  count: " << in_pages_B[1] << endl;

// OPENCL HOST CODE AREA START

    vector<cl::Device> devices = get_devices();
    cl::Device device = devices[0];
    string device_name = device.getInfo<CL_DEVICE_NAME>();
    cout << "Found Device=" << device_name.c_str() << endl;

    //Creating Context and Command Queue for selected device
    cl::Context context(device);
    // Wenqi: enable out of order execution
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);

    // Import XCLBIN
    xclbin_file_name = argv[1];
    cl::Program::Binaries vadd_bins = import_binary_file();

    // Program and Kernel
    devices.resize(1);
    cl::Program program(context, devices, vadd_bins);
    cl::Kernel krnl_scheduler(program, "scheduler");
    cl::Kernel krnl_executor(program, "executor");

    cout << "Finish loading bitstream...\n";
    // in init 
    OCL_CHECK(err, cl::Buffer buffer_in_pages_A   (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            bytes_page_A, in_pages_A.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in_pages_B   (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            bytes_page_B, in_pages_B.data(), &err));
    // in & out
    OCL_CHECK(err, cl::Buffer buffer_layer_cache(context,CL_MEM_USE_HOST_PTR, 
            layer_cache_bytes, layer_cache.data(), &err));
	// out
    OCL_CHECK(err, cl::Buffer buffer_out(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, 
            out_bytes, out.data(), &err));

    cout << "Finish allocate buffer...\n";

    // int arg_counter = 0;    
    // in 
    OCL_CHECK(err, err = krnl_scheduler.setArg(0, int(max_level_A)));
    OCL_CHECK(err, err = krnl_scheduler.setArg(1, int(max_level_B)));

    OCL_CHECK(err, err = krnl_executor.setArg(0, int(max_level_A)));
    OCL_CHECK(err, err = krnl_executor.setArg(1, int(max_level_B)));
    OCL_CHECK(err, err = krnl_executor.setArg(2, int(root_id_A)));
    OCL_CHECK(err, err = krnl_executor.setArg(3, int(root_id_B)));
    OCL_CHECK(err, err = krnl_executor.setArg(4, int(page_bytes)));
    OCL_CHECK(err, err = krnl_executor.setArg(12, buffer_in_pages_A));
    OCL_CHECK(err, err = krnl_executor.setArg(13, buffer_in_pages_B));
    OCL_CHECK(err, err = krnl_executor.setArg(17, buffer_layer_cache));
    OCL_CHECK(err, err = krnl_executor.setArg(18, buffer_out));


    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
        // in
        buffer_in_pages_A,
        buffer_in_pages_B,
        buffer_layer_cache,
        buffer_out
        },0/* 0 means from host*/));
    q.finish();

    // wait_for_enter("\nPress ENTER to continue after setting up ILA trigger...");
    cout << "Launching kernel...\n";
    // Launch the Kernel
    auto start = chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(krnl_scheduler));
    OCL_CHECK(err, err = q.enqueueTask(krnl_executor));
    q.finish();
    
    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_out},CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    auto end = chrono::high_resolution_clock::now();
    double duration = (chrono::duration_cast<chrono::milliseconds>(end-start).count() / 1000.0);

    cout << "Duration (including memcpy out): " << duration << " sec" << endl; 

    if (out[0] == correct_intersect) {
        cout << endl << "Result correct!" << endl;
    } else {
        cout << endl << "Result wrong!" << endl;
    }
    cout << "Parsed intersect count : " << correct_intersect << endl;
    cout << "FPGA computed intersect count : " << out[0] << endl;

    for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {
        int task_count = out[1 + PE_id];
        cout << "PE " << PE_id << " computes " << task_count << " page joins" << endl;
    }
    
    return  0;
}
