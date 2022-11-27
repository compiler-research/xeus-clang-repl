# Copyright (c) Jupyter Development Team.
# Distributed under the terms of the Modified BSD License.

# https://hub.docker.com/r/jupyter/base-notebook/tags
ARG BASE_CONTAINER=jupyter/base-notebook:ubuntu-20.04
FROM $BASE_CONTAINER

LABEL maintainer="Xeus-clang-repl Project"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

USER root

ENV TAG="ubuntu-20.04"

RUN export

# Install all OS dependencies for notebook server that starts but lacks all
# features (e.g., download as all possible file formats)
RUN apt-get update --yes && \
    apt-get install --yes --no-install-recommends \
    #fonts-liberation, pandoc, run-one are inherited from base-notebook container image
    # Other "our" apt installs
    unzip \
    curl \
    jq \
    # Other "our" apt installs (development and testing)
    build-essential \
    git \
    nano-tiny \
    less \
    && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

USER ${NB_UID}

# Copy git repository to home directory of container
COPY --chown=${NB_UID}:${NB_GID} . "${HOME}"/

# Jupyter Notebook, Lab, and Hub are installed in base image
# ReGenerate a notebook server config
# Cleanup temporary files
# Correct permissions
# Do all this in a single RUN command to avoid duplicating all of the
# files across image layers when the permissions change
WORKDIR /tmp
RUN mamba install --quiet --yes -c conda-forge \
    # notebook,jpyterhub, jupyterlab are inherited from base-notebook container image
    # Other "our" conda installs
    cmake \
    #'clangdev=15' \
    'xeus>=2.0,<3.0' \
    'nlohmann_json>=3.9.1,<3.10' \
    'cppzmq>=4.6.0,<5' \
    'xtl>=0.7,<0.8' \
    pugixml \
    'cxxopts>=2.1.1,<2.2' \
    libuuid \
    # Test dependencies
    pytest \
    jupyter_kernel_test \
    && \
    jupyter notebook --generate-config -y && \
    mamba clean --all -f -y && \
    npm cache clean --force && \
    jupyter lab clean && \
    rm -rf "/home/${NB_USER}/.cache/yarn" && \
    fix-permissions "${CONDA_DIR}" && \
    fix-permissions "/home/${NB_USER}"

EXPOSE 8888

# Configure container startup
CMD ["start-notebook.sh"]

USER root

# Make /home/runner directory and fix permisions
RUN mkdir /home/runner && fix-permissions /home/runner

# Switch back to jovyan to avoid accidental container runs as root
USER ${NB_UID}

ENV NB_PYTHON_PREFIX=${CONDA_DIR} \
    KERNEL_PYTHON_PREFIX=${CONDA_DIR} \
    CPLUS_INCLUDE_PATH="${CONDA_DIR}/include:/home/${NB_USER}/include:/home/runner/work/xeus-clang-repl/xeus-clang-repl/clang-dev/clang/include"

WORKDIR "${HOME}"

### Post Build
RUN \
    # Install clang-dev
    artifact_name="clang-dev" && \
    arr=(${BINDER_REQUEST//\// }) && \
    gh_repo_owner=${arr[2]} && \
    gh_repo_name=${arr[3]} && \
    gh_repo="${repo_owner}/${repo_name}" && \
    gh_repo_branch=${arr[4]} && \
    #
    echo ${gh_repo_owner} && \
    echo ${gh_repo_name} && \
    echo ${gh_repo} && \
    echo ${gh_repo_branch} && \
    #
    repository_id=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_repo_owner}/${gh_repo_name}" | jq -r ".id") && \
    echo ${repository_id} && \
    artifacts_info=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/compiler-research/${gh_repo_name}/actions/artifacts?per_page=100&name=${artifact_name}") && \
    artifact_id=$(echo "$artifacts_info" | jq -r "[.artifacts[] | select(.expired == false and .workflow_run.head_repository_id == ${repository_id} and .workflow_run.head_branch == \"${gh_repo_branch}\")] | sort_by(.updated_at)[-1].id") && \
    download_url="https://nightly.link/compiler-research/xeus-clang-repl/actions/artifacts/${artifact_id}.zip" && \
    mkdir -p /home/runner/work/xeus-clang-repl/xeus-clang-repl && \
    pushd /home/runner/work/xeus-clang-repl/xeus-clang-repl && \
    curl "$download_url" -L -o "${artifact_name}.zip" && \
    unzip "${artifact_name}.zip" && \
    rm "${artifact_name}.zip" && \
    tar xjf ${artifact_name}.tar.bz2 && \
    rm ${artifact_name}.tar.bz2 && \
    cd $artifact_name && \
    PATH_TO_CLANG_DEV=`pwd` && \
    popd && \
    #
    PATH_TO_LLVM_BUILD=$PATH_TO_CLANG_DEV/build && \
    export PATH=$PATH_TO_LLVM_BUILD/bin:$PATH && \
    export LD_LIBRARY_PATH=$PATH_TO_LLVM_BUILD/lib:$LD_LIBRARY_PATH && \
    # Build and Install xeus-clang-repl
    mkdir build && \
    cd build && \
    cmake -DLLVM_CMAKE_DIR=$PATH_TO_LLVM_BUILD -DCMAKE_PREFIX_PATH=$KERNEL_PYTHON_PREFIX -DCMAKE_INSTALL_PREFIX=$KERNEL_PYTHON_PREFIX -DCMAKE_INSTALL_LIBDIR=lib -DLLVM_CONFIG_EXTRA_PATH_HINTS=${PATH_TO_LLVM_BUILD}/lib -DLLVM_USE_LINKER=gold .. && \
    make install -j$(nproc --all)
