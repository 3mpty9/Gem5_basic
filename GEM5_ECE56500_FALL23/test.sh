scons-3 USE_HDF5=0 -j `nproc` ./build/ECE565-ARM/gem5.opt

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b sjeng --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache