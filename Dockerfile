# ---- Build Stage ----
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Grab everything we need to compile: the C++ toolchain, CMake, and all
# the MongoDB C/C++ client libraries with their SSL dependencies.
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
    libcurl4-openssl-dev

# Build the C++ MongoDB driver from source since the Ubuntu repo version
# is usually too old and missing features we rely on (like Stable API v1).
WORKDIR /tmp
RUN wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz && \
    tar -xzf mongo-cxx-driver-r3.10.1.tar.gz && \
    cd mongo-cxx-driver-r3.10.1/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -GNinja && \
    cmake --build . --target install

# Copy our project files and compile everything.
WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -GNinja && \
    cmake --build . -j$(nproc)


# ---- Runtime Stage ----
FROM ubuntu:24.04

# Pull in the MongoDB C++ driver libraries we built earlier.
COPY --from=builder /usr/local/lib/libmongocxx* /usr/local/lib/
COPY --from=builder /usr/local/lib/libbsoncxx* /usr/local/lib/

# Install only what's needed at runtime: SSL, the C driver, and curl
# for Firebase token validation.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    libssl3 \
    libmongoc-1.0-0 \
    libbson-1.0-0 \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Make sure the dynamic linker can find our custom-built libraries.
RUN ldconfig

WORKDIR /app

# Copy the compiled binary from the build stage.
COPY --from=builder /app/build/healthlink_backend /app/healthlink_backend

# These get injected at runtime via docker-compose or Kubernetes secrets.
# They're listed here as documentation of what the app expects.
ENV MONGO_URI="mongodb+srv://Admin:VIbiE0XZFTGsPa2K@cluster0.9iugzbv.mongodb.net/?appName=Cluster0"
ENV PORT="18080"
ENV FIREBASE_PROJECT_ID="bugs-producers"
ENV FIREBASE_API_KEY="AIzaSyBUQeCqxwBJp_Li5xL2BjUK-yUQmKa34RY"
ENV FIREBASE_AUTH_DOMAIN="bugs-producers.firebaseapp.com"
ENV FIREBASE_STORAGE_BUCKET="bugs-producers.firebasestorage.app"
ENV FIREBASE_MESSAGING_SENDER_ID="780566146913"
ENV FIREBASE_APP_ID="1:780566146913:web:6866ab39f3f3c4c4569f20"
ENV FIREBASE_MEASUREMENT_ID="G-G415PTCQ4D"

# Expose the Crow API port.
EXPOSE 18080

CMD ["./healthlink_backend"]