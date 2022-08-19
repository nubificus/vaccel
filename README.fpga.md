- In plugins/fpga -> included fpga related plugins - uses the following prerequisities:

https://github.com/ikwzm/udmabuf
https://www.xilinx.com/products/design-tools/legacy-tools/sdsoc.html or SDSoc or equivlaent for HLS tools for Xilinx only! (only for parralel.c example)

- In examples/fpga_examples -> contains examples for each of the plugins:
-=> Needs SDSoc for the parallel one but also may depend on the base bitstream used for the FPGA fabric:

- Working for the current FPGA SoC Image:
https://github.com/ikwzm/FPGA-SoC-Linux 

- May work for Xilinx/PYNQ petalinux but some tweaks may be required

- Other tweak is the src/resources.c (line 172 ```"%s/resource.%llu"``` (llu changed from lu)


See also:


https://github.com/jl2022s/fpga-vaccel-tutorial-repo
https://github.com/jl2022s/fpga_samples



First link for using Lab 3 for sample1 and sample2 of udambuf example.
Second for C examples without Vaccel.


TODO: > Dividing example
