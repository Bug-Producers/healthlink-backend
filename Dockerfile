# Build stage
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies including MongoDB C/C++ drivers
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
    ninja-build

# Build and install mongocxx (since Ubuntu repo version might be old)
WORKDIR /tmp
RUN wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.10.1.tar.gz && \
    cd mongo-cxx-driver-r3.10.1/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -GNinja && \
    cmake --build . --target install

# Set up the actual project workspace
WORKDIR /app
COPY . .

# Build our project (Using the new required CMake version >= 3.28)
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja && \
    cmake --build . -j$(nproc)

# Final stage (Runtime)
FROM ubuntu:24.04

# Copy necessary runtime libraries from the builder
COPY --from=builder /usr/local/lib/libmongocxx* /usr/local/lib/
COPY --from=builder /usr/local/lib/libbsoncxx* /usr/local/lib/

# Install runtime dependencies (OpenSSL, standard C++ libs, C drivers)
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    libssl3 \
    libmongoc-1.0-0 \
    libbson-1.0-0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Update dynamic linker
RUN ldconfig

WORKDIR /app
# Copy the compiled executable from the builder stage
COPY --from=builder /app/build/healthlink_backend /app/healthlink_backend

# Expose Crow API port
EXPOSE 18080

CMD ["./healthlink_backend"]
