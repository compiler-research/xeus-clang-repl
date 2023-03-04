# Copyright (c) Jupyter Development Team.
# Distributed under the terms of the Modified BSD License.

# https://hub.docker.com/r/jupyter/base-notebook/tags
ARG BASE_CONTAINER=jupyter/base-notebook
#ARG BASE_TAG=latest
#ARG BASE_TAG=ubuntu-22.04
#TODO: Next line is temporary workaround.
#      Remove when we can build xeus-clang-repl with Xeus>=3.0
ARG BASE_TAG=7285848c0a11
#ARG BASE_TAG=2023-01-24
#ARG BASE_TAG=python-3.10.6
FROM $BASE_CONTAINER:$BASE_TAG

LABEL maintainer="Xeus-clang-repl Project"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

USER root

ENV TAG="$BASE_TAG"

ENV LC_ALL=en_US.UTF-8 \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8
    
# Install all OS dependencies for notebook server that starts but lacks all
# features (e.g., download as all possible file formats)
RUN #apt-get update --yes && \
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
    net-tools \
    && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
    
# Create alternative for nano -> nano-tiny
RUN update-alternatives --install /usr/bin/nano nano /bin/nano-tiny 10

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
RUN #mamba update --all --quiet --yes -c conda-forge && \
    mamba install --quiet --yes -c conda-forge \
    # notebook,jpyterhub, jupyterlab are inherited from base-notebook container image
    # Other "our" conda installs
    cmake \
    #'clangdev=15' \
    'xeus>=2.0,<3.0' \
    'nlohmann_json>=3.9.1,<3.10' \
    'cppzmq>=4.6.0,<5' \
    'xtl>=0.7,<0.8' \
    pugixml \
    'cxxopts>=2.2.1,<2.3' \
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

RUN sudo netstat -a

# Switch back to jovyan to avoid accidental container runs as root
USER ${NB_UID}

ENV NB_PYTHON_PREFIX=${CONDA_DIR} \
    KERNEL_PYTHON_PREFIX=${CONDA_DIR} \
    CPLUS_INCLUDE_PATH="${CONDA_DIR}/include:/home/${NB_USER}/include:/home/runner/work/xeus-clang-repl/xeus-clang-repl/clang-dev/clang/include"

WORKDIR "${HOME}"

### Post Build
RUN \
    #
    # Install clang-dev from GH Artifact or Release asset
    #
    artifact_name="clang-dev" && \
    git_remote_origin_url=$(git config --get remote.origin.url) && \
    echo "Debug: Remote origin url: $git_remote_origin_url" && \
    arr=(${git_remote_origin_url//\// }) && \
    gh_repo_owner=${arr[2]} && \
    gh_f_repo_owner="compiler-research" && \
    arr=(${arr[3]//./ }) && \
    gh_repo_name=${arr[0]} && \
    gh_repo="${gh_repo_owner}/${gh_repo_name}" && \
    gh_f_repo_name=${gh_repo_name} && \
    h=$(git rev-parse HEAD) && \
    echo "Debug: Head h: $h" && \
    br=$(git branch) && \
    echo "Debug: Branch br: $br" && \
    arr1=$(git show-ref --head | grep $h | grep -E "remotes|tags" | grep -o '[^/ ]*$') && \
    gh_repo_branch="${arr1[*]//\|}" && \
    gh_repo_branch_regex=" ${gh_repo_branch//$'\n'/ | } " && \
    gh_repo_branch_regex=$(echo "$gh_repo_branch_regex" | sed -e 's/[]\/$*.^[]/\\\\&/g') && \
    echo "Debug: Repo Branch: $gh_repo_branch" && \
    echo "Debug: Repo Branch Regex: $gh_repo_branch_regex" && \
    #
    mkdir -p /home/runner/work/xeus-clang-repl/xeus-clang-repl && \
    pushd /home/runner/work/xeus-clang-repl/xeus-clang-repl && \
    # repo
    repository_id=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_repo_owner}/${gh_repo_name}" | jq -r ".id") && \
    echo "Debug: Repo id: $repository_id" && \
    artifacts_info=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_repo_owner}/${gh_repo_name}/actions/artifacts?per_page=100&name=${artifact_name}") && \
    artifact_id=$(echo "$artifacts_info" | jq -r "[.artifacts[] | select(.expired == false and .workflow_run.repository_id == ${repository_id} and (\" \"+.workflow_run.head_branch+\" \" | test(\"${gh_repo_branch_regex}\")))] | sort_by(.updated_at)[-1].id") && \
    download_url="https://nightly.link/${gh_repo_owner}/${gh_repo_name}/actions/artifacts/${artifact_id}.zip" && \
    # forked repo
    f_repository_id=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_f_repo_owner}/${gh_f_repo_name}" | jq -r ".id") && \
    echo "Debug: Forked Repo id: $f_repository_id" && \
    f_artifacts_info=$(curl -s -H "Accept: application/vnd.github+json" "https://api.github.com/repos/${gh_f_repo_owner}/${gh_f_repo_name}/actions/artifacts?per_page=100&name=${artifact_name}") && \
    f_artifact_id=$(echo "$f_artifacts_info" | jq -r "[.artifacts[] | select(.expired == false and .workflow_run.repository_id == ${f_repository_id} and (\" \"+.workflow_run.head_branch+\" \" | test(\"${gh_repo_branch_regex}\")))] | sort_by(.updated_at)[-1].id") && \
    f_download_url="https://nightly.link/${gh_f_repo_owner}/${gh_f_repo_name}/actions/artifacts/${f_artifact_id}.zip" && \
    # tag
    for download_tag in $gh_repo_branch; do echo "Debug: try tag $download_tag:"; download_tag_url="https://github.com/${gh_repo_owner}/${gh_repo_name}/releases/download/${download_tag}/${artifact_name}.tar.bz2"; if curl --head --silent --fail -L $download_tag_url 1>/dev/null; then echo "found"; break; fi; done && \
    # try to download artifact ot release tag asset
    echo "Debug: Download url (asset) repo info: $download_tag_url" && \
    echo "Debug: Download url (artifact) repo info: $download_url" && \
    echo "Debug: Download url (artifact) forked repo info: $f_download_url" && \
    if curl --head --silent --fail -L $download_tag_url 1>/dev/null; then curl "$download_tag_url" -L -o "${artifact_name}.tar.bz2"; elif curl --head --silent --fail -L $download_url 1>/dev/null; then curl "$download_url" -L -o "${artifact_name}.zip"; else curl "$f_download_url" -L -o "${artifact_name}.zip"; fi && \
    if [[ -f "${artifact_name}.zip" ]]; then unzip "${artifact_name}.zip"; rm "${artifact_name}.zip"; fi && \
    tar xjf ${artifact_name}.tar.bz2 && \
    rm ${artifact_name}.tar.bz2 && \
    cd $artifact_name && \
    PATH_TO_CLANG_DEV=$(pwd) && \
    popd && \
    #
    PATH_TO_LLVM_BUILD=$PATH_TO_CLANG_DEV/build && \
    export PATH=$PATH_TO_LLVM_BUILD/bin:$PATH && \
    export LD_LIBRARY_PATH=$PATH_TO_LLVM_BUILD/lib:$LD_LIBRARY_PATH && \
    #
    # Build and Install xeus-clang-repl
    #
    mkdir build && \
    cd build && \
    cmake -DLLVM_CMAKE_DIR=$PATH_TO_LLVM_BUILD -DCMAKE_PREFIX_PATH=$KERNEL_PYTHON_PREFIX -DCMAKE_INSTALL_PREFIX=$KERNEL_PYTHON_PREFIX -DCMAKE_INSTALL_LIBDIR=lib -DLLVM_CONFIG_EXTRA_PATH_HINTS=${PATH_TO_LLVM_BUILD}/lib -DLLVM_REQUIRED_VERSION=15 -DLLVM_USE_LINKER=gold .. && \
    make install -j$(nproc --all) && \
    cd .. && \
    #
    # Build and Install Clad
    #
    mkdir clad && \
    cd clad && \
    git clone --depth=1 https://github.com/vgvassilev/clad.git && \
    mkdir build && \
    cd build && \
    cmake ../clad -DClang_DIR=${PATH_TO_LLVM_BUILD}/lib/cmake/clang/ -DLLVM_DIR=${PATH_TO_LLVM_BUILD}/lib/cmake/llvm/ -DCMAKE_INSTALL_PREFIX=/opt/conda -DLLVM_EXTERNAL_LIT="$(which lit)" && \
    #make -j$(nproc --all) && \
    make && \
    make install && \
    #
    ls -la &&\
    export && \
    ls /usr/local/bin -la
