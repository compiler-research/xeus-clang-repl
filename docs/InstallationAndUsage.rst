InstallationAndUsage
--------------------

You will first need to install dependencies.

.. code-block:: bash

    mamba install cmake cxx-compiler nlohmann_json cppzmq xtl jupyterlab 
    clangdev=14 cxxopts pugixml -c conda-forge


**Note:** Use a mamba environment with python version >= 3.11 for fetching clang-versions.

The safest usage is to create an environment named `xeus-clang-repl`.

.. code-block:: bash

    mamba create -n  xeus-clang-repl
    source activate  xeus-clang-repl


Installing from conda-forge:
Then you can install in this environment `xeus-clang-repl` and its dependencies.

.. code-block:: bash

    mamba install xeus-clang-repl notebook -c conda-forge

.. code-block:: bash

    git clone https://github.com/llvm/llvm-project

    git checkout -b release/15.0x

    git apply patches/llvm/clang15-D127284.patch

    mkdir build

    cd build

    cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm

    make -j n

    cd ..

    git clone https://github.com/compiler-research/xeus-clang-repl.git

    mkdir build

    cd build

    # The clang project which we have built above has to be given in the LLVM path
    cmake ../ -DClang_DIR=/usr/lib/llvm-15/build/lib/cmake/clang\
            -DLLVM_DIR=/usr/lib/llvm-15/build/lib/cmake/llvm

    make -j n
