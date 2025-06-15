FROM ubuntu:20.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libboost-all-dev \
    libpqxx-dev \
    libpq-dev \
    libssl-dev \
    tzdata \
    bash \
    dos2unix \
    libasio-dev \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app


RUN chmod +x /app/scripts/*.sh

RUN bash /app/scripts/install_lib.sh

RUN bash /app/scripts/clean.sh || true

RUN bash /app/scripts/make.sh

RUN bash /app/scripts/compile.sh

EXPOSE 8080

CMD ["/app/release/bin/ship_management_system"]
