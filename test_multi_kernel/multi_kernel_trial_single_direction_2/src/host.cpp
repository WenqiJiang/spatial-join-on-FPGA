#include <stdint.h>
#include <chrono>

#include "host.hpp"

#include "constants.hpp"
// Wenqi: seems 2022.1 somehow does not support linking ap_uint.h to host?
// #include "ap_uint.h"


int main(int argc, char** argv)
{
    cl_int err;
    // Allocate Memory in Host Memory
    // When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the hood user ptr 
    // is used if it is properly aligned. when not aligned, runtime had no choice but to create
    // its own host side buffer. So it is recommended to use this allocator if user wish to
    // create buffer using CL_MEM_USE_HOST_PTR to align user buffer to page boundary. It will 
    // ensure that user buffer is used when user create Buffer/Mem object with CL_MEM_USE_HOST_PTR 

    std::cout << "Allocating memory...\n";


    size_t bytes_input = 1024;
    std::vector<int ,aligned_allocator<int>> input(bytes_input);

    size_t bytes_output = 1024; // no more than 16 GB
    std::vector<int ,aligned_allocator<int>> output(bytes_output / sizeof(int));

    input[0] = 999;

// OPENCL HOST CODE AREA START

    std::vector<cl::Device> devices = get_devices();
    cl::Device device = devices[0];
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    //Creating Context and Command Queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device);

    // Import XCLBIN
    xclbin_file_name = argv[1];
    cl::Program::Binaries vadd_bins = import_binary_file();

    // Program and Kernel
    devices.resize(1);
    cl::Program program(context, devices, vadd_bins);
    cl::Kernel krnl_A(program, "PE_A");
    cl::Kernel krnl_B(program, "PE_B");

    std::cout << "Finish loading bitstream...\n";
    // in init 
    OCL_CHECK(err, cl::Buffer buffer_input   (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            bytes_input, input.data(), &err));

	// out
    OCL_CHECK(err, cl::Buffer buffer_output(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, 
            bytes_output, output.data(), &err));

    std::cout << "Finish allocate buffer...\n";

    int iter = 100;
    // int arg_counter_A = 0;    
    OCL_CHECK(err, err = krnl_A.setArg(0, iter));
    OCL_CHECK(err, err = krnl_A.setArg(1, buffer_input));

    // int arg_counter_B = 0;    
    OCL_CHECK(err, err = krnl_B.setArg(0, iter));
    OCL_CHECK(err, err = krnl_B.setArg(2, buffer_output));


    // Copy input data to device global memory
    // OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
    //     // in
    //     buffer_input,
    //     // buffer_output
    //     }, 0/* 0 means from host*/));

    std::cout << "Launching kernel...\n";
    // Launch the Kernel
    auto start = std::chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(krnl_A));
    // Wenqi: the result is only correct if we call finish here, but this is not suppose to be 
    //    right for streaming kernels...
    // q.finish(); 

    // OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
    //     // in
    //     // buffer_input,
    //     buffer_output
    //     }, 0/* 0 means from host*/));
    OCL_CHECK(err, err = q.enqueueTask(krnl_B));
    q.finish();

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output},CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    auto end = std::chrono::high_resolution_clock::now();
    double duration = (std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() / 1000.0);

    std::cout << "Duration (including memcpy out): " << duration << " sec" << std::endl; 

    std::cout << "First element of output: " << output[0] << std::endl;
    
    std::cout << "TEST FINISHED (NO RESULT CHECK)" << std::endl; 

    return  0;
}
