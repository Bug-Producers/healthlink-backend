FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libmongoc-1.0-0 \
    libbson-1.0-0 \
    libmongoc-dev \
    libbson-dev \
    wget \
    ninja-build \
    libcurl4-openssl-dev \
    libasio-dev

WORKDIR /tmp

RUN wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.10.1.tar.gz && \
    cd mongo-cxx-driver-r3.10.1/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -GNinja && \
    cmake --build . --target install

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja && \
    cmake --build . -j$(nproc)

FROM ubuntu:24.04

COPY --from=builder /usr/local/lib/libmongocxx* /usr/local/lib/
COPY --from=builder /usr/local/lib/libbsoncxx* /usr/local/lib/

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libssl3 \
    libmongoc-1.0-0 \
    libbson-1.0-0 \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN ldconfig

WORKDIR /app

COPY --from=builder /app/build/healthlink_backend /app/healthlink_backend

EXPOSE 18080

CMD ["./healthlink_backend"]