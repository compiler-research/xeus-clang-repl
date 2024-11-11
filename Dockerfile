# Copyright (c) Jupyter Development Team.
# Distributed under the terms of the Modified BSD License.

# https://hub.docker.com/r/jupyter/base-notebook/tags
ARG BASE_CONTAINER=jupyter/base-notebook
#ARG BASE_TAG=latest
ARG BASE_TAG=ubuntu-22.04
#TODO: Next ARG line(s) is temporary workaround.
#      Remove them when we can build xeus-clang-repl with Xeus>=3.0
#ARG BASE_TAG=7285848c0a11
#ARG BASE_TAG=2023-01-24
#ENV VENV_PYTHON_VERSION=3.10.6
ARG BASE_TAG=python-3.10.6
FROM $BASE_CONTAINER:$BASE_TAG

ARG BUILD_TYPE=Release

LABEL maintainer="Xeus-clang-repl Project"
#LABEL com.nvidia.volumes.needed="nvidia_driver"

SHELL ["/bin/bash", "--login", "-o", "pipefail", "-c"]

USER root

ENV TAG="$BASE_TAG"

ENV LC_ALL=en_US.UTF-8 \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8

# Install all OS dependencies for notebook server that starts but lacks all
# features (e.g., download as all possible file formats)
RUN \
    set -x && \
    apt-get update --yes && \
    apt-get install --yes --no-install-recommends pciutils && \
    lspci && \
    export _CUDA_="$(lspci -nn | grep '\[03' | grep NVIDIA)" && \
    apt-get install --yes --no-install-recommends \
      #fonts-liberation, pandoc, run-one are inherited from base-notebook container image
    # Other "our" apt installs
      unzip \
      curl \
      jq \
      ###libomp-dev \
    # Other "our" apt installs (development and testing)
      build-essential \
      #FIXME: Update to clang-17 once available in apt repository (expected with next LTS version of Ubuntu April 2024)
      #       to be use a consistent version of clang throughout
      clang-15 \
      git \
      nano-tiny \
      less \
      gdb valgrind \
      emacs \
    # CUDA
      #cuda \
      $([ -n "$_CUDA_" ] && echo nvidia-cuda-toolkit) \
    && \
    apt-get clean &&  \
    apt-get autoremove && \
    rm -rf /var/lib/apt/lists/* && \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && \
    locale-gen

ENV LC_ALL=en_US.UTF-8 \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8

# Create alternative for nano -> nano-tiny
#RUN update-alternatives --install /usr/bin/nano nano /bin/nano-tiny 10

USER ${NB_UID}

# Copy git repository to home directory of container
COPY --chown=${NB_UID}:${NB_GID} . "${HOME}"/

EXPOSE 8888 8889

# Configure container startup
CMD ["start-notebook.sh", "--debug", "&>/home/jovyan/log.txt"]

USER root

# Fix start-notebook.sh
RUN sed -i '2 i source /home/jovyan/.conda.init && conda activate .venv' /usr/local/bin/start-notebook.sh

# Make /home/runner directory and fix permisions
RUN mkdir /home/runner && fix-permissions /home/runner

# Switch back to jovyan to avoid accidental container runs as root
USER ${NB_UID}

ENV NB_PYTHON_PREFIX=${CONDA_DIR} \
    KERNEL_PYTHON_PREFIX=${CONDA_DIR} \
    CPLUS_INCLUDE_PATH="${CONDA_DIR}/include:/home/${NB_USER}/include:/home/runner/work/xeus-clang-repl/xeus-clang-repl/clang-dev/clang/include:/home/jovyan/clad/include:/home/jovyan/CppInterOp/include"

WORKDIR "${HOME}"

# CUDA
ENV NVIDIA_VISIBLE_DEVICES=all
ENV NVIDIA_DRIVER_CAPABILITIES=compute,utility
ENV NVIDIA_REQUIRE_CUDA "cuda>=12.1.1 driver>=530"

# VENV

# Jupyter Notebook, Lab, and Hub are installed in base image
# ReGenerate a notebook server config
# Cleanup temporary files
# Correct permissions
# Do all this in a single RUN command to avoid duplicating all of the
# files across image layers when the permissions change
#RUN mamba update --all --quiet --yes -c conda-forge && \
RUN \
    set -x && \
    # setup virtual environment
    mamba create -y -n .venv python=3.10.6 && \
    #
    #echo "echo \"@ @ @  PROFILE @ @ @ \"" >> ~/.profile && \
    #echo "echo \"@ @ @  BASHRC @ @ @ \"" >> /home/jovyan/.bashrc && \
    mv /home/jovyan/.bashrc /home/jovyan/.bashrc.tmp && \
    touch /home/jovyan/.bashrc && \
    conda init bash && \
    mv /home/jovyan/.bashrc /home/jovyan/.conda.init && \
    mv /home/jovyan/.bashrc.tmp /home/jovyan/.bashrc && \
    conda init bash && \
    echo "source /home/jovyan/.conda.init && conda activate .venv" >> /home/jovyan/.bashrc && \
    #
    source /home/jovyan/.conda.init && \
    conda activate .venv && \
    fix-permissions "${CONDA_DIR}" && \
    #
    mamba install --quiet --yes -c conda-forge \
    # notebook, jpyterhub, jupyterlab are inherited from base-notebook container image
    # Other "our" conda installs
    cmake \
    #"clangdev=$LLVM_REQUIRED_VERSION" \
    'xeus>=2.0' \
    xeus-zmq \
    'nlohmann_json>=3.9.1,<3.10' \
    'cppzmq>=4.6.0,<5' \
    'xtl>=0.7,<0.8' \
    'openssl<4' \
    ipykernel \
    pugixml \
    zlib \
    libxml2 \
    'cxxopts>=2.2.1,<2.3' \
    libuuid \
    # Test dependencies
    pytest \
    jupyter_kernel_test \
    && \
    hash -r && \
    pip install ipython && \
    #rm /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    jupyter notebook --generate-config -y && \
    mamba clean --all -f -y && \
    npm cache clean --force && \
    jupyter lab clean && \
    rm -rf "/home/${NB_USER}/.cache/yarn" && \
    fix-permissions "${CONDA_DIR}" && \
    fix-permissions "/home/${NB_USER}"

### Post Build
RUN \
    set -x && \
    source /home/jovyan/.conda.init && \
    conda activate .venv && \
    export ARCHITECHURE=$(uname -m) && \
    if [ "$ARCHITECHURE" == "x86_64" ] ; \ 
    then \
    #
    artifact_name="clang-dev" && \
    #
    # Install clang-dev from GH Artifact or Release asset
    #
    echo $PWD && git_remote_origin_url=$(git config --get remote.origin.url) && \
    echo "Debug: Remote origin url: $git_remote_origin_url" && \
    arr=(${git_remote_origin_url//\// }) && \
    arr2=(${arr[0]//:/ }) && \
    gh_repo_owner=${arr2[1]} && \
    gh_f_repo_owner="compiler-research" && \
    arr=(${arr[1]//./ }) && \
    gh_repo_name=${arr[0]} && \
    gh_repo="${gh_repo_owner}/${gh_repo_name}" && \
    gh_f_repo_name=${gh_repo_name} && \
    h=$(git rev-parse HEAD) && \
    echo "Debug: Head h: $h" && \
    #FIXME: if `$h` is not pushed upstream this fails. We should just diagnose and move on.
    git show-ref --head && echo $? && \
    git show-ref --head | grep "$h" && echo $? && \
    git show-ref --head | grep "$h" | grep -E "remotes|tags" && echo $? && \
    git show-ref --head | grep "$h" | grep -E "remotes|tags" | grep -o '[^/ ]*$' && echo $? && \
    arr1=$(git show-ref --head | grep "$h" | grep -E "remotes|tags" | grep -o '[^/ ]*$') && echo $? && \
    gh_repo_branch="${arr1[*]//\|}" && \
    gh_repo_branch_regex=" ${gh_repo_branch//$'\n'/ | } " && \
    gh_repo_branch_regex=$(echo "$gh_repo_branch_regex" | sed -e 's/[]\/$*.^[]/\\\\&/g') && \
    echo "Debug: Repo Branch: $gh_repo_branch" && \
    echo "Debug: Repo Branch Regex: $gh_repo_branch_regex" && \
    mkdir -p /home/runner/work/xeus-clang-repl/xeus-clang-repl && \
    pushd /home/runner/work/xeus-clang-repl/xeus-clang-repl && \
    # repo
    echo "Debug: Repo owner/name: ${gh_repo_owner} / ${gh_repo_name}" && \
#RUN \
#    set -x && \
    source /home/jovyan/.conda.init && \
    conda activate .venv && \
    repository_id=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_repo}" | jq -r ".id") && \
    echo "Debug: Repo id: $repository_id" && \
    artifacts_info=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_repo}/actions/artifacts?per_page=100&name=${artifact_name}") && \
    artifact_id=$(echo "$artifacts_info" | jq -r "[.artifacts[] | select(.expired == false and .workflow_run.repository_id == ${repository_id} and (\" \"+.workflow_run.head_branch+\" \" | test(\"${gh_repo_branch_regex}\")))] | sort_by(.updated_at)[-1].id") && \
    #download_url="https://nightly.link/${gh_repo_owner}/${gh_repo_name}/actions/artifacts/${artifact_id}.zip" && \
    download_url="https://link-to.alexander-penev.info/${gh_repo_owner}/${gh_repo_name}/actions/artifacts/${artifact_id}.zip" && \
    # forked repo
    f_repository_id=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_f_repo_owner}/${gh_f_repo_name}" | jq -r ".id") && \
    echo "Debug: Forked Repo id: $f_repository_id" && \
    f_artifacts_info=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_f_repo_owner}/${gh_f_repo_name}/actions/artifacts?per_page=100&name=${artifact_name}") && \
    f_artifact_id=$(echo "$f_artifacts_info" | jq -r "[.artifacts[] | select(.expired == false and .workflow_run.repository_id == ${f_repository_id} and (\" \"+.workflow_run.head_branch+\" \" | test(\"${gh_repo_branch_regex}\")))] | sort_by(.updated_at)[-1].id") && \
    #f_download_url="https://nightly.link/${gh_f_repo_owner}/${gh_f_repo_name}/actions/artifacts/${f_artifact_id}.zip"
    f_download_url="https://link-to.alexander-penev.info/${gh_f_repo_owner}/${gh_f_repo_name}/actions/artifacts/${f_artifact_id}.zip" && \
    # tag
    for download_tag in $gh_repo_branch; do echo "Debug: try tag $download_tag:"; download_tag_url="https://github.com/${gh_repo_owner}/${gh_repo_name}/releases/download/${download_tag}/${artifact_name}.tar.bz2"; if curl --head --silent --fail -L $download_tag_url 1>/dev/null; then echo "found"; break; fi; done && \
    # try to download artifact ot release tag asset
    echo "Debug: Download url (asset) repo info: $download_tag_url" && \
    echo "Debug: Download url (artifact) repo info: $download_url" && \
    echo "Debug: Download url (artifact) forked repo info: $f_download_url" && \
    ###
    ### clang-dev gihub only:
    if curl --head --silent --fail -L "$download_tag_url" 1>/dev/null; then curl "$download_tag_url" -L -o "${artifact_name}.tar.bz2"; elif curl --head --silent --fail -L "$download_url" 1>/dev/null; then curl "$download_url" -L -o "${artifact_name}.zip"; else curl "$f_download_url" -L -o "${artifact_name}.zip"; fi && \
    ### clang-dev local archive only:
    # mv /home/jovyan/"${artifact_name}.zip" ./&& \
    ###
    if [[ -f "${artifact_name}.zip" ]]; then unzip "${artifact_name}.zip"; rm "${artifact_name}.zip"; fi && \
    tar xjf ${artifact_name}.tar.bz2 && \
    rm ${artifact_name}.tar.bz2 && \
    cd $artifact_name && \
    export PATH_TO_CLANG_DEV=$(pwd) && \
    popd && \
    #
    echo "Debug clang path: $PATH_TO_CLANG_DEV" && \
    export PATH_TO_LLVM_BUILD=$PATH_TO_CLANG_DEV/inst/ && \
    export PATH_TO_LLVM_CONFIG=$PATH_TO_CLANG_DEV/inst/lib/cmake/llvm/ && \
    export PATH_TO_CLANG_CONFIG=$PATH_TO_CLANG_DEV/inst/lib/cmake/clang/ ; \
    fi  && \
    export VENV=${CONDA_DIR}/envs/.venv && \
    echo "export VENV=$VENV" >> ~/.profile && \
    echo "export EDITOR=emacs" >> ~/.profile && \
    #
    # Build LLVM (if on arm) & CppInterOp 
    #  
    sys_incs=$(LC_ALL=C c++ -xc++ -E -v /dev/null 2>&1 | LC_ALL=C sed -ne '/starts here/,/End of/p' | LC_ALL=C sed '/^ /!d' | cut -c2- | tr '\n' ':') && \
    export CPLUS_INCLUDE_PATH="${PATH_TO_LLVM_BUILD}/include/llvm:${PATH_TO_LLVM_BUILD}/include/clange:$CPLUS_INCLUDE_PATH:${sys_incs%:}" && \
    git clone https://github.com/compiler-research/CppInterOp.git && \
    if [ "$ARCHITECHURE" == "aarch64" ] ; \ 
    then \
    #FIXME: Update to clang-17 once clang-17 is available in apt repository (expected with next LTS version of Ubuntu 
    #       around April 2024)
    export CC=/usr/bin/clang-15 && \
    export CXX=/usr/bin/clang++-15 && \
    #FIXME: Currently build with version 16.x on arm due to issue mentioned here https://github.com/compiler-research/xeus-clang-repl/issues/78
	git clone  --depth=1 --branch release/16.x  https://github.com/llvm/llvm-project.git && \
	cd ./llvm-project/ && \
	mkdir build && \
	export LLVM_DIR=$(pwd) && \
    compgen -G "../CppInterOp/patches/llvm/clang16-*.patch" > /dev/null && find ../CppInterOp/patches/llvm/clang16-*.patch -printf "%f\n" && git apply ../CppInterOp/patches/llvm/clang16-*.patch && \
    cd build && \
    cmake -DLLVM_ENABLE_PROJECTS=clang -DLLVM_TARGETS_TO_BUILD="host;NVPTX" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_USE_LINKER=gold -DCLANG_ENABLE_STATIC_ANALYZER=OFF -DCLANG_ENABLE_ARCMT=OFF -DCLANG_ENABLE_FORMAT=OFF -DCLANG_ENABLE_BOOTSTRAP=OFF ../llvm && \
    cmake --build . --target clang clang-repl --parallel $(nproc --all) && \
    export PATH_TO_LLVM_CONFIG=$LLVM_DIR/build/lib/cmake/llvm/ && \
    export PATH_TO_CLANG_CONFIG=$LLVM_DIR/build/lib/cmake/clang/ && \
    export PATH_TO_LLVM_BUILD=$LLVM_DIR/build && \
    cd ../../ ; \
    fi && \
    export PATH=${VENV}/bin:${CONDA_DIR}/bin:$PATH_TO_LLVM_BUILD/bin:$PATH && \
    export LD_LIBRARY_PATH=$PATH_TO_LLVM_BUILD/lib:$LD_LIBRARY_PATH && \
    echo "export PATH=$PATH" >> ~/.profile && \
    export CB_PYTHON_DIR="$PWD/cppyy-backend/python" && \
    export CPPINTEROP_DIR="$CB_PYTHON_DIR/cppyy_backend" && \
    mkdir CppInterOp/build && \
    cd CppInterOp/build && \
    export CPPINTEROP_BUILD_DIR=$PWD && \
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DUSE_CLING=OFF -DUSE_REPL=ON -DLLVM_DIR=$PATH_TO_LLVM_CONFIG -DClang_DIR=$PATH_TO_CLANG_CONFIG -DLLVM_USE_LINKER=gold -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$CPPINTEROP_DIR .. && \
    cmake --build . --parallel $(nproc --all) && \
    #make install -j$(nproc --all)
    export CPLUS_INCLUDE_PATH="$CPPINTEROP_DIR/include:$CPLUS_INCLUDE_PATH" && \
    export LD_LIBRARY_PATH="${VENV}/lib:${CONDA_DIR}/lib:$CPPINTEROP_DIR/lib:$LD_LIBRARY_PATH" && \
    echo "export LD_LIBRARY_PATH=$CPPINTEROP_DIR/lib:$LD_LIBRARY_PATH" >> ~/.profile && \
    cd ../.. && \
    #
    # Build and Install cppyy-backend
    #
    git clone --depth=1 https://github.com/compiler-research/cppyy-backend.git && \
    cd cppyy-backend && \
    mkdir -p $CPPINTEROP_DIR/lib build && \
    cd build && \
    # Install CppInterOp
    (cd $CPPINTEROP_BUILD_DIR && cmake --build . --target install --parallel $(nproc --all)) && \
    # Build and Install cppyy-backend
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCppInterOp_DIR=$CPPINTEROP_DIR .. && \
    cmake --build . --parallel $(nproc --all) && \
    cp libcppyy-backend.so $CPPINTEROP_DIR/lib/ && \
    cd ../.. && \
    #
    # Build and Install CPyCppyy
    #
    # Install CPyCppyy
    git clone --depth=1 https://github.com/compiler-research/CPyCppyy.git && \
    mkdir CPyCppyy/build && \
    cd CPyCppyy/build  && \
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && \
    cmake --build . --parallel $(nproc --all) && \
    export CPYCPPYY_DIR=$PWD && \
    cd ../.. && \
    #
    # Build and Install cppyy
    #
    # Install cppyy
    git clone --depth=1 https://github.com/compiler-research/cppyy.git && \
    cd cppyy && \
    python -m pip install --upgrade . --no-deps && \
    cd .. && \
    # Run cppyy
    #TODO: Fix cppyy path (/home/jovyan) to path to installed module
    export PYTHONPATH=$PYTHONPATH:$CPYCPPYY_DIR:$CB_PYTHON_DIR:/home/jovyan && \
    echo "export PYTHONPATH=$PYTHONPATH" >> ~/.profile && \
    export CPLUS_INCLUDE_PATH="/home/jovyan/CPyCppyy/include/:$CPLUS_INCLUDE_PATH" && \
    export PYTHON_VERSION=$(python --version | cut -c8- | cut -f1,2 -d'.') && \
    export CPLUS_INCLUDE_PATH="/home/jovyan/clad/include:$CPLUS_INCLUDE_PATH" && \
    export CPLUS_INCLUDE_PATH="${VENV}/include:${VENV}/include/python${PYTHON_VERSION}:$CPLUS_INCLUDE_PATH" && \
    python -c "import cppyy" && \
    #
    # Build and Install xeus-clang-repl
    #
    mkdir build && \
    cd build && \
    echo "export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH" >> ~/.profile && \
    ##echo "conda activate .venv" >> ~/.profile
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DLLVM_CMAKE_DIR=$PATH_TO_LLVM_BUILD -DCMAKE_PREFIX_PATH=$KERNEL_PYTHON_PREFIX -DCMAKE_INSTALL_PREFIX=$KERNEL_PYTHON_PREFIX -DCMAKE_INSTALL_LIBDIR=lib -DLLVM_CONFIG_EXTRA_PATH_HINTS=${PATH_TO_LLVM_BUILD}/lib -DCPPINTEROP_DIR=$CPPINTEROP_BUILD_DIR -DLLVM_USE_LINKER=gold .. && \
    make install -j$(nproc --all) && \
    cd .. && \
    #
    # Build and Install Clad
    #
    git clone --depth=1 https://github.com/vgvassilev/clad.git && \
    mkdir clad/build && \
    cd clad/build && \
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. -DClang_DIR=$PATH_TO_CLANG_CONFIG -DLLVM_DIR=$PATH_TO_LLVM_CONFIG -DCMAKE_INSTALL_PREFIX=${CONDA_DIR} -DLLVM_EXTERNAL_LIT="$(which lit)" && \
    make -j$(nproc --all) && \
    make install -j$(nproc --all) && \
    ### install clad in all exist kernels
    ##for i in "$KERNEL_PYTHON_PREFIX"/share/jupyter/kernels/*; do if [[ $i =~ .*/clad-xcpp.* ]]; then jq '.argv += ["-fplugin=$KERNEL_PYTHON_PREFIX/lib/clad.so"] | .display_name += " (with clad)"' "$i"/kernel.json > tmp.$$.json && mv tmp.$$.json "$i"/kernel.json; fi; done && \
    ###
    ### Add OpenMP to all kernels
    ###
    ##for i in "$KERNEL_PYTHON_PREFIX"/share/jupyter/kernels/*; do if [[ $i =~ .*/xcpp.* ]]; then jq '.argv += ["-fopenmp"] | .display_name += " (with OpenMP)"' "$i"/kernel.json > tmp.$$.json && mv tmp.$$.json "$i"/kernel.json; fi; done && \
    #
    # CUDA
    #
    #echo "c = get_config()" >> /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    #echo "c.NotebookApp.notebook_manager_class = 'jupyter_gpu.GPUNotebookManager'" >> /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    cat /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    echo "c.GPUNotebookManager.gpu_device = 0" >> /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    # Web password and token set to ""
    echo "c.NotebookApp.token = ''" >> /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    echo "c.NotebookApp.password = ''" >> /home/jovyan/.jupyter/jupyter_notebook_config.py && \
    # Patch /opt/conda/share/jupyter/kernels/python3/kernel.json to use .venv
    k="/opt/conda/share/jupyter/kernels/python3/kernel.json" && \
    jq ".argv[0] = \"${VENV}/bin/python\"" $k > $k.$$.tmp && mv $k.$$.tmp $k && \
    # xtensor
    git clone --depth=1 https://github.com/xtensor-stack/xtensor.git && \
    mkdir xtensor/build && \
    cd xtensor/build && \
    cmake -DCMAKE_INSTALL_PREFIX=$KERNEL_PYTHON_PREFIX .. && \
    make install -j$(nproc --all)
