# FPGA Performance test scripts

Run a single test (two datasets): perf_test.py

Run all experiments (various datasets, max node entry size, etc.): run_all_experiments.py

```
python run_all_experiments.py \
--out_json_fname FPGA_perf_1_PE_3_runs.json \
--overwrite 0 \
--FPGA_project_dir /mnt/scratch/wenqi/spatial-join-on-FPGA/FPGA_BFS/BFS_multi_PE_v2.6_1_PE \
--cpp_exe_dir /mnt/scratch/wenqi/spatial-join-baseline/cpp/FPGA_index_constructor \
--get_tree_depth_py_dir /mnt/scratch/wenqi/spatial-join-baseline/python/get_tree_depth.py \
--C_file_dir /mnt/scratch/wenqi/spatial-join-baseline/generated_data \
--num_runs 3 > log_run_all_experiments


python run_all_experiments.py \
--out_json_fname FPGA_perf_2_PE_3_runs.json \
--overwrite 0 \
--FPGA_project_dir /mnt/scratch/wenqi/spatial-join-on-FPGA/FPGA_BFS/BFS_multi_PE_v2.6_2_PE \
--cpp_exe_dir /mnt/scratch/wenqi/spatial-join-baseline/cpp/FPGA_index_constructor \
--get_tree_depth_py_dir /mnt/scratch/wenqi/spatial-join-baseline/python/get_tree_depth.py \
--C_file_dir /mnt/scratch/wenqi/spatial-join-baseline/generated_data \
--num_runs 3 > log_run_all_experiments

python run_all_experiments.py \
--out_json_fname FPGA_perf_4_PE_3_runs.json \
--overwrite 0 \
--FPGA_project_dir /mnt/scratch/wenqi/spatial-join-on-FPGA/FPGA_BFS/BFS_multi_PE_v2.6_4_PE \
--cpp_exe_dir /mnt/scratch/wenqi/spatial-join-baseline/cpp/FPGA_index_constructor \
--get_tree_depth_py_dir /mnt/scratch/wenqi/spatial-join-baseline/python/get_tree_depth.py \
--C_file_dir /mnt/scratch/wenqi/spatial-join-baseline/generated_data \
--num_runs 3 > log_run_all_experiments


python run_all_experiments.py \
--out_json_fname FPGA_perf_8_PE_3_runs.json \
--overwrite 0 \
--FPGA_project_dir /mnt/scratch/wenqi/spatial-join-on-FPGA/FPGA_BFS/BFS_multi_PE_v2.6_8_PE \
--cpp_exe_dir /mnt/scratch/wenqi/spatial-join-baseline/cpp/FPGA_index_constructor \
--get_tree_depth_py_dir /mnt/scratch/wenqi/spatial-join-baseline/python/get_tree_depth.py \
--C_file_dir /mnt/scratch/wenqi/spatial-join-baseline/generated_data \
--num_runs 3 > log_run_all_experiments

python run_all_experiments.py \
--out_json_fname FPGA_perf_16_PE_3_runs.json \
--overwrite 0 \
--FPGA_project_dir /mnt/scratch/wenqi/spatial-join-on-FPGA/FPGA_BFS/BFS_multi_PE_v2.6_16_PE \
--cpp_exe_dir /mnt/scratch/wenqi/spatial-join-baseline/cpp/FPGA_index_constructor \
--get_tree_depth_py_dir /mnt/scratch/wenqi/spatial-join-baseline/python/get_tree_depth.py \
--C_file_dir /mnt/scratch/wenqi/spatial-join-baseline/generated_data \
--num_runs 3 > log_run_all_experiments
```
