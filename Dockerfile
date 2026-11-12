# Adaptado do ambiente_pp da disciplina de Programação Paralela.
FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        gdb \
        valgrind \
        libomp-dev \
        python3 \
        python3-pip \
        time \
        nano \
        less \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /lab
CMD ["/bin/bash"]
