#!/bin/bash
jupyter-repo2docker \
    --user-name=jovyan \
    --image-name xeus-clang-repl \
    --publish 8888 \
    .

#    --editable \
#    --ref InterOpIntegration \
#    https://github.com/alexander-penev/xeus-clang-repl.git \
