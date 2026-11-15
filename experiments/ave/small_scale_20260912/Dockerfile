FROM ubuntu:22.04

ARG CUDD_COMMIT=20857e7b5f0e4c806888a807032a3c6eee849ac1

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=/workspace/FactorGraph/build/out/blif_solve_lib:/workspace/FactorGraph/build/out/factor_graph:/workspace/FactorGraph/build/out/dd:/workspace/cudd-3.0.0/cudd/.libs

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
        automake \
        build-essential \
        ca-certificates \
        cmake \
        git \
        libtool \
        libz-dev \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

RUN git clone https://github.com/appu226/cudd.git cudd-3.0.0 \
    && git -C cudd-3.0.0 checkout "$CUDD_COMMIT" \
    && cd cudd-3.0.0 \
    && ./configure ACLOCAL=aclocal AUTOMAKE=automake --enable-shared --disable-static --enable-dddmp \
    && make -j2

COPY . /workspace/FactorGraph
WORKDIR /workspace/FactorGraph

RUN cmake -S . -B build/out -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build/out --target ave_main ave2_main --parallel 2

ENTRYPOINT ["python3", "jan_24/run_ave_small_resume.py"]
CMD ["--help"]