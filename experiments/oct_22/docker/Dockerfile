FROM ubuntu:jammy-20230605

RUN apt-get update
RUN  apt-get install -y git gcc g++ sudo make automake cmake libz-dev

RUN mkdir /home/data_sets

RUN mkdir /home/fguser
RUN useradd -d /home/fguser fguser
RUN chown -R fguser /home/fguser
USER fguser
WORKDIR /home/fguser
RUN cd /home/fguser


RUN git clone https://github.com/appu226/cudd.git cudd-3.0.0
RUN cd cudd-3.0.0 && ./configure ACLOCAL=aclocal AUTOMAKE=automake --enable-dddmp && make -j check

RUN git clone https://github.com/appu226/FactorGraph.git FactorGraph
RUN cd FactorGraph && cmake -S . -B build/out -DCMAKE_BUILD_TYPE=RelWithDebInfo && make -j -C build/out && build/out/test/test1

WORKDIR /home/fguser/FactorGraph/experiments/oct_22

ENTRYPOINT ["run_chunk.sh"]
