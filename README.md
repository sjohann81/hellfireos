# HellfireOS Realtime Operating System

optional: 
- Download GNU cross (MIPS) toolchain (4.6 series, pre-built)
  - [32 bits](https://dl.dropboxusercontent.com/u/7936618/gcc-4.6.1_x86.tar.gz)
  - [64 bits](https://dl.dropboxusercontent.com/u/7936618/gcc-4.6.1.tar.gz)
```sh
$ export PATH=$PATH:<PATH_TO_TOOLCHAIN>/bin/
```
 - Execute 
```sh
$ git clone git@github.com/sjohan81/hellfireos.git
$ cd /usr/sim/hf_risc/
$ gcc hf_risc_sim.c -o hf_risc_sim
$ cd /platform/single_core
$ make image
$ ./usr/sim/hf_risc_sim/hf_risc_sim platform/single_core/image.bin 
```