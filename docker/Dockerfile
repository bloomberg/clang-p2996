# Multi-arch Dockerfile (parametrized by ARCH)
# Build on a native host for the selected architecture.
# Keep docker/Dockerfile.arm64 and docker/Dockerfile.amd64 intact.

# ========= Builder =========
FROM ubuntu:24.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
ARG ARCH=arm64

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates build-essential git cmake ninja-build python3 \
    libxml2-dev zlib1g-dev libedit-dev libncurses-dev pkg-config \
    lld curl file \
 && rm -rf /var/lib/apt/lists/*

# Clone bloomberg fork with Reflection for C++26 (P2996)
ARG CLANG_P2996_REF=p2996
WORKDIR /src
RUN git clone --depth=1 --branch ${CLANG_P2996_REF} https://github.com/bloomberg/clang-p2996.git

WORKDIR /src/clang-p2996
RUN mkdir build-p2996-linux
WORKDIR /src/clang-p2996/build-p2996-linux

# Configure, build and install depending on ARCH
RUN set -eux; \
    if [ "$ARCH" = "amd64" ]; then \
      LLVM_TGT=X86; \
    else \
      LLVM_TGT=AArch64; \
    fi; \
    cmake -G Ninja \
      ../llvm \
      -DLLVM_TARGETS_TO_BUILD=${LLVM_TGT} \
      -DLLVM_ENABLE_PROJECTS="clang" \
      -DLLVM_ENABLE_RUNTIMES="libunwind;libcxxabi;libcxx;compiler-rt" \
      -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/p2996/clang \
      -DLLVM_USE_LINKER=lld \
      -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
      -DCLANG_DEFAULT_LINKER=lld \
      -DCLANG_DEFAULT_RTLIB=compiler-rt \
      -DLIBCXX_USE_COMPILER_RT=ON \
      -DLIBCXXABI_USE_COMPILER_RT=ON \
      -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
      -DLIBUNWIND_USE_COMPILER_RT=ON; \
    ninja -j"$(nproc)" && ninja install

# ========= Runner =========
FROM ubuntu:24.04 AS runner

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates build-essential git pkg-config libxml2 zlib1g libtinfo6 \
 && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/p2996/clang /opt/p2996/clang

ENV PATH=/opt/p2996/clang/bin:$PATH \
    CC=/opt/p2996/clang/bin/clang \
    CXX=/opt/p2996/clang/bin/clang++ \
    LD_LIBRARY_PATH=/opt/p2996/clang/lib

WORKDIR /work

CMD ["bash"]

