FROM ubuntu:jammy-20230605

RUN apt-get update
RUN  apt-get install -y git gcc g++ sudo make automake cmake libz-dev

RUN mkdir /home/data_sets \
    && mkdir /home/fguser \
    && useradd -d /home/fguser fguser \
    && chown -R fguser /home/fguser
USER fguser
WORKDIR /home/fguser


RUN git clone https://github.com/appu226/cudd.git cudd-3.0.0 \
    && cd cudd-3.0.0 \
    && ./configure ACLOCAL=aclocal AUTOMAKE=automake --enable-dddmp \
    && make -j check

RUN git clone https://github.com/appu226/FactorGraph.git FactorGraph \
    && cd FactorGraph \
    && cmake -S . -B build/out -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && make -j -C build/out && build/out/test/test1
RUN cd FactorGraph && git pull && echo 3vms

WORKDIR /home/fguser/FactorGraph/experiments/oct_22

ENTRYPOINT ["./run_chunk.sh"]
