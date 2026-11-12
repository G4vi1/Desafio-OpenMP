FROM ubuntu:24.04

# GCC fornece OpenMP; Matplotlib exporta os graficos para PNG e SVG.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        python3 \
        python3-matplotlib \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /lab
COPY codigos/ ./codigos/
COPY scripts/ ./scripts/
COPY dados_exemplo.txt README.md requirements-graficos.txt ./
COPY scripts/docker-entrypoint.sh /usr/local/bin/desafio-entrypoint

# Compila dentro da imagem, sem reutilizar executaveis do computador.
RUN chmod +x /usr/local/bin/desafio-entrypoint \
    && make -C codigos all

# Mesmo UID/GID do usuario local para gravar relatorios sem exigir sudo depois.
ARG USER_UID=1000
ARG USER_GID=1000
RUN chown -R ${USER_UID}:${USER_GID} /lab
USER ${USER_UID}:${USER_GID}

ENV OMP_DYNAMIC=FALSE \
    MPLCONFIGDIR=/tmp/matplotlib
ENTRYPOINT ["/usr/local/bin/desafio-entrypoint"]
CMD ["python3", "scripts/comparar.py", "--repeticoes", "30"]
