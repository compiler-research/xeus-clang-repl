# Copyright (c) Jupyter Development Team.
# Distributed under the terms of the Modified BSD License.

# https://hub.docker.com/r/jupyter/base-notebook/tags
ARG BASE_CONTAINER=jupyter/base-notebook
ARG BASE_TAG=latest
#ARG BASE_TAG=ubuntu-22.04
#TODO: Next line is temporary workaround.
#      Remove when we can build xeus-clang-repl with Xeus>=3.0
#ARG BASE_TAG=ed2908bbb62e
#ARG BASE_TAG=python-3.10.8
FROM $BASE_CONTAINER:$BASE_TAG

#LABEL maintainer="Xeus-clang-repl Project"
