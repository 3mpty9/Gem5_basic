scons-3 USE_HDF5=0 -j `nproc` ./build/ECE565-ARM/gem5.opt

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b sjeng --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache&&mkdir 0/sjeng&&cp -r m5out/stats.txt 0/sjeng/stats.txt&&cp -r m5out/config.ini 0/sjeng/config.ini&&cp -r m5out/config.json 0/sjeng/config.json

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b leslie3d --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache&&mkdir 0/leslie3d&&cp -r m5out/stats.txt 0/leslie3d/stats.txt&&cp -r m5out/config.ini 0/leslie3d/config.ini&&cp -r m5out/config.json 0/leslie3d/config.json

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b lbm --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache&&mkdir 0/lbm&&cp -r m5out/stats.txt 0/lbm/stats.txt&&cp -r m5out/config.ini 0/lbm/config.ini&&cp -r m5out/config.json 0/lbm/config.json

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b astar --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache&&mkdir 0/astar&&cp -r m5out/stats.txt 0/astar/stats.txt&&cp -r m5out/config.ini 0/astar/config.ini&&cp -r m5out/config.json 0/astar/config.json

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b milc --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache&&mkdir 0/milc&&cp -r m5out/stats.txt 0/milc/stats.txt&&cp -r m5out/config.ini 0/milc/config.ini&&cp -r m5out/config.json 0/milc/config.json

./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py --cpu-type=MinorCPU -b namd --maxinsts=10000000 --l1d_size=64kB --l1i_size=16kB --caches --l2cache&&mkdir 0/namd&&cp -r m5out/stats.txt 0/namd/stats.txt&&cp -r m5out/config.ini 0/namd/config.ini&&cp -r m5out/config.json 0/namd/config.json


