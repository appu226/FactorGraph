FROM ubuntu:jammy-20240111

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
    && ./configure ACLOCAL=aclocal AUTOMAKE=automake --enable-dddmp --enable-shared \
    && make -j check

RUN git clone https://github.com/appu226/kissat.git kissat \
    && cd kissat \
    && git checkout cav_2024 \
    && ./configure \
    && make -j test

RUN git clone https://github.com/appu226/FactorGraph.git FactorGraph \
    && cd FactorGraph \
    && cmake -S . -B build/out -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && make -j -C build/out && build/out/test/test1


WORKDIR /home/fguser/FactorGraph/experiments/jan_24

ENTRYPOINT ["./run_chunk.sh"]
