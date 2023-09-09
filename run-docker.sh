#!/bin/bash
docker container run --rm -i hadolint/hadolint hadolint - < Dockerfile

jupyter-repo2docker \
    --no-run \
    --user-name=jovyan \
    --image-name xeus-clang-repl \
    .

#docker run --gpus all --publish 8888:8888 --name xeus-clang-repl-c -i -t xeus-clang-repl "start-notebook.sh"
docker run --rm --runtime=nvidia --gpus all --publish 8888:8888 --name xeus-clang-repl-c -i -t xeus-clang-repl "start-notebook.sh"

#    --editable \
#    --ref InterOpIntegration \
#    https://github.com/alexander-penev/xeus-clang-repl.git \
