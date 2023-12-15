## Try it online

To try out xeus-clang-repl interactively in your web browser, just click on the binder
link:

[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/compiler-research/xeus-clang-repl/v0.2.0?labpath=notebooks/index.ipynb)

### Xeus-Clang-REPL

## Description

`xeus-clang-repl` integrates clang-repl with the xeus protocol and is a platform for C++ usage in Jupyter Notebooks. The demo developed in this repository shows a Python - CPP integraton in Jupyter Notebooks, where variables can be transfered between Python and CPP.

`Disclaimer: this work is highly experimental and might not work beyond the examples provided`

## Installation

To ensure that the installation works, it is preferable to install `xeus-clang-repl` in a
fresh environment. It is also needed to use a
[miniforge](https://github.com/conda-forge/miniforge#mambaforge) or
[miniconda](https://conda.io/miniconda.html) installation because with the full
[anaconda](https://www.anaconda.com/) you may have a conflict with the zeromq library 

You will first need to install dependencies

```bash
mamba install xeus xeus-zmq cmake cxx-compiler nlohmann_json cppzmq xtl jupyterlab clangdev=14 cxxopts pugixml -c conda-forge
```

**Note:** Use a mamba environment with python version >= 3.11 for fetching clang-versions

The safest usage is to create an environment named `xeus-clang-repl`

```bash
mamba create -n  `xeus-clang-repl`
source activate  `xeus-clang-repl`
```

<!-- ### Installing from conda-forge

Then you can install in this environment `xeus-clang-repl` and its dependencies

```bash
mamba install`xeus-clang-repl` notebook -c conda-forge
``` -->

```bash
git clone --depth=1 --branch release/15.0x https://github.com/llvm/llvm-project

cd llvm-project

git apply patches/llvm/clang15-D127284.patch

mkdir build

cd build

cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm

make -j n

cd ..

git clone https://github.com/compiler-research/xeus-clang-repl.git

mkdir build

cd build

cmake ../ -DClang_DIR=/usr/lib/llvm-15/build/lib/cmake/clang\
         -DLLVM_DIR=/usr/lib/llvm-15/build/lib/cmake/llvm

make -j n
```

## Docker
It is possible to build xeus-clang-repl within a Docker container. On an arm based Apple machine with docker, docker-compose and colima installed, you can try out xeus-clang-repl using the following commands (cpu represents number of cores, and memory is in GiB, so adjust according to your machine)
```bash
git clone https://github.com/compiler-research/xeus-clang-repl.git
cd xeus-clang-repl
colima start --cpu 8 --memory 8 --arch aarch64 --vm-type=vz --vz-rosetta
chmod a+x ./build-and-run-docker.sh
./build-and-run-docker.sh
```
Once the bash script finishes running, you can experiment with xeus-clang-repl by opening up your browser and visiting http://127.0.0.1:8888/lab


## Try it online (for developers)

To try out xeus-clang-repl interactively in your web browser, just click on the binder
link (Warning! This is Nightly build):

[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/compiler-research/xeus-clang-repl/HEAD?labpath=notebooks/index.ipynb)


<img
   src="./integration-demo-readme.png"
   alt="Alt text"
   title="Optional title"
   style="display: block; margin: 0 auto; max-width: 450px">

## Documentation

To get started with using `xeus-clang-repl`, The Documentation work is under Development.

## Dependencies

`xeus-clang-repl` depends on

- [xtl](https://github.com/xtensor-stack/xtl)
- [nlohmann_json](https://github.com/nlohmann/json)
- [cppzmq](https://github.com/zeromq/cppzmq)
- [clang](https://github.com/llvm/llvm-project/)
- [cxxopts](https://github.com/jarro2783/cxxopts)

|   `xeus-clang-repl`   |       `xtl`     |  `clang`  | `pugixml` | `cppzmq` | `cxxopts` | `nlohmann_json` | `dirent` (windows only) |
|-----------------------|-----------------|-----------|-----------|----------|-----------|-----------------|-------------------------|
|   	~0.1.0 		      |  >=0.7.0,<0.8.0 | >=16,<17  | ~1.8.1    | ~4.3.0   |  >=3.0.0  |  >=3.6.1,<4.0   |    >=2.3.2,<3           |

## License

This software is licensed under the `Apache License`. See the [LICENSE](LICENSE)
file for details.
