FROM ubuntu:24.10

RUN apt-get update
RUN apt-get install --assume-yes git vim build-essential libboost-program-options-dev libreadline-dev libgmp-dev libssl-dev python3 python3-venv python3-pybind11 cmake
RUN git clone https://github.com/Kitware/CMake.git cmake_src
RUN git clone https://github.com/meelgroup/manthan
RUN cd manthan && git submodule update --init --recursive

## HACK to insert a missing #include in manthan-preprocess
## ideally someone should fix it in git
RUN sed --in-place s/#include\ \<algorithm\>/#include\ \<algorithm\>\\n#include\ \<cstdint\>/ /manthan/dependencies/manthan-preprocess/louvain-community/src/graph_plain.h

WORKDIR manthan
RUN ["/bin/bash", "-c", "python3 -m venv manthan-venv && source manthan-venv/bin/activate && python -m pip install -r requirements.txt"]
RUN ["/bin/bash", "-c", "source manthan-venv/bin/activate && chmod +x configure_dependencies.sh && ./configure_dependencies.sh --all"]

COPY docker_entrypoint.sh /manthan
RUN mkdir /workspace

ENTRYPOINT ["/manthan/docker_entrypoint.sh"]

