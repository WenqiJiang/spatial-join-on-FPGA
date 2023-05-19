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

    int max_level_A = 20;
    int max_level_B = 20;
    int root_id_A = 0;
    int root_id_B = 0;
    int page_bytes = 4096;

    string tree_parent_dir = "/mnt/scratch/wenqi/spatial-join-baseline/cpp";
	string tree_A_fname = "tree_A.bin";
	string tree_B_fname = "tree_B.bin";
    string tree_bin_A_dir = dir_concat(tree_parent_dir, tree_A_fname);
    string tree_bin_B_dir = dir_concat(tree_parent_dir, tree_B_fname);

    cout << "Parsed tree level A : " << max_level_A << endl;
    cout << "Parsed tree level B : " << max_level_B << endl;

    // get data size from disk 
    ifstream tree_A_fstream(
        tree_bin_A_dir, ios::in | ios::binary);
    tree_A_fstream.seekg(0, tree_A_fstream.end);
    int64_t tree_A_bytes = tree_A_fstream.tellg();
    tree_A_fstream.seekg(0, tree_A_fstream.beg);
    cout << "Tree A bytes: " << tree_A_bytes << endl;

    ifstream tree_B_fstream(
        tree_bin_B_dir, ios::in | ios::binary);
    tree_B_fstream.seekg(0, tree_B_fstream.end);
    int64_t tree_B_bytes = tree_B_fstream.tellg();
    tree_B_fstream.seekg(0, tree_B_fstream.beg);
    cout << "Tree B bytes: " << tree_B_bytes << endl;

    // allocate memory 
    cout << "Allocating memory...\n";
    size_t bytes_page_A = tree_A_bytes;
    size_t bytes_page_B = tree_B_bytes;
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
    tree_A_fstream.read((char*) in_pages_A.data(), tree_A_bytes);
    if (!tree_A_fstream) {
            cout << "error: only " << tree_A_fstream.gcount() << " could be read";
        exit(1);
    }
    tree_B_fstream.read((char*) in_pages_B.data(), tree_B_bytes);
    if (!tree_B_fstream) {
            cout << "error: only " << tree_B_fstream.gcount() << " could be read";
        exit(1);
    }

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

    cout << "FPGA computed intersect count : " << out[0] << endl;

    for (int PE_id = 0; PE_id < N_JOIN_PE; PE_id++) {
        int task_count = out[1 + PE_id];
        cout << "PE " << PE_id << " computes " << task_count << " page joins" << endl;
    }
    
    return  0;
}
