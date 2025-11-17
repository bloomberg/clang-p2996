## Table of Contents

- [What is this repository?](#what-is-this-repository)
- [Additional Resources](#additional-resources)
- [Pre-built Docker images for Clang with Reflection](#pre-built-docker-images-for-clang-with-reflection)
  - [How to download the pre-built images from Docker Hub](#how-to-download-the-pre-built-images-from-docker-hub)
  - [How to check the clang version in the pre-built images](#how-to-check-the-clang-version-in-the-pre-built-images)
  - [How to verify that Clang in the pre-built images builds viable executable](#how-to-verify-that-clang-in-the-pre-built-images-builds-viable-executable)
  - [How to run examples from the P2996 proposal using pre-built images](#how-to-run-examples-from-the-p2996-proposal-using-pre-built-images)
- [How to build Docker image for Clang with Reflection](#how-to-build-docker-image-for-clang-with-reflection)
- [How to build and use Clang with Reflection on macOS M-series](#how-to-build-and-use-clang-with-reflection-on-macos-m-series)
  - [How to build the Clang with Reflection on macOS M-series](#how-to-build-the-clang-with-reflection-on-macos-m-series)
  - [How to check the Clang with Reflection version](#how-to-check-the-clang-with-reflection-version)
  - [How to verify that Clang with Reflection builds viable executable](#how-to-verify-that-clang-with-reflection-builds-viable-executable)
  - [How to run examples from the P2996 proposal using the Clang with Reflection](#how-to-run-examples-from-the-p2996-proposal-using-the-clang-with-reflection)

## What is this repository?

This repository is a fork of LLVM's `llvm-project` repository, created to host a fork of the
[clang](https://clang.llvm.org) compiler front-end implementing experimental support for ISO C++
proposal [P2996 ("_Reflection for C++26_")](https://wg21.link/p2996). Support for a handful of
related proposals is also implemented ([P1036 ("Expansion Statements")](https://wg21.link/p1306),
[P3096 ("Function Parameter Reflection in Reflection for C++26")](https://wg21.link/p3096),
[P3394 ("Annotations for Reflection")](https://wg21.link/p3394),
[P3491 ("`define_static_{string,object,array}`")](https://wg21.link/p3491)), which can be enabled
with the `-freflection-latest` flag. Development primarily
takes place on the [p2996](https://github.com/bloomberg/clang-p2996/tree/p2996) branch; you can
learn more about this fork in our [project documentation](
https://github.com/bloomberg/clang-p2996/tree/p2996/P2996.md).

P2996, and the proposals listed above, were adopted into C++26 during the 2025 Sofia meeting of
WG21. Although this fork remains the most complete existing implementation of P2996 as of June
2025, we expect for the development of production-grade implementations in upstream compilers to
soon begin. With the caveat that this implementation of P2996 is and always has been experimental
(and often made choices in the name of expediency, or easier merge conflict resolution, than would
be made by an upstream implementation), the contributors to this fork hope that it may serve as a
starting point for would-be implementers of P2996 in the upstream LLVM project.

The Clang/P2996 fork is highly experimental; sharp edges abound and occasional crashes should be
expected. Memory usage has not been optimized and is, in many cases, wasteful. **DO NOT use this
project to build any artifacts destined for production.**

## Additional Resources

- Project docs: https://github.com/bloomberg/clang-p2996/tree/p2996/P2996.md
- LLVM Getting Started: https://llvm.org/docs/GettingStarted.html
- Docker Hub image: https://hub.docker.com/r/vsavkov/clang-p2996
- Gazing Beyond Reflection for C++26 — Daveed Vandevoorde (CppCon 2024): https://www.youtube.com/watch?v=wpjiowJW2ks
- Three Cool Things in C++26: Safety, Reflection & std::execution — Herb Sutter (C++ on Sea 2025): https://www.youtube.com/watch?v=kKbT0Vg3ISw
- Trip report: June 2025 ISO C++ standards meeting (Sofia): https://herbsutter.com/2025/06/
- C++ Weekly — Ep 504 — Practical Reflection in C++26: https://www.youtube.com/watch?v=Mg_TBYppQwU

## Pre-built Docker images for Clang with Reflection

These images contain Clang with experimental C++26 Reflection support and libc++ runtimes, for native hosts:

- linux/arm64: `vsavkov/clang-p2996:arm64`
- linux/amd64: `vsavkov/clang-p2996:amd64`

### How to download the pre-built images from Docker Hub

```
docker pull vsavkov/clang-p2996:arm64
docker pull vsavkov/clang-p2996:amd64
```

### How to check the clang version in the pre-built images

Use the Makefile targets, overriding `IMAGE_NAME` to point at the Docker Hub repo:

```
make IMAGE_NAME=vsavkov/clang-p2996 docker-clang-version-arm64
make IMAGE_NAME=vsavkov/clang-p2996 docker-clang-version-amd64
```

### How to verify that Clang in the pre-built images builds viable executable

Use the Makefile targets to compile and run the checker inside the container:

```
make IMAGE_NAME=vsavkov/clang-p2996 docker-clang-check-arm64
make IMAGE_NAME=vsavkov/clang-p2996 docker-clang-check-amd64
```

### How to run examples from the P2996 proposal using pre-built images

Use the Makefile targets to compile and run `p2996_examples.cpp` inside the container:

```
make IMAGE_NAME=vsavkov/clang-p2996 docker-clang-p2996-examples-arm64
make IMAGE_NAME=vsavkov/clang-p2996 docker-clang-p2996-examples-amd64
```

## How to build Docker image for Clang with Reflection

Use the unified Dockerfile via the Makefile build targets (run on a native host for the target architecture):

```
make docker-build-arm64
make docker-build-amd64
```

## How to build and use Clang with Reflection on macOS M-series

### How to build the Clang with Reflection on macOS M-series

The project documentation refers to the LLVM Getting Started guide for building `clang` and `libc++`.  
Below are simplified instructions for building `clang` and `libc++` for C++26 with Reflection on macOS M-series.

Clone `bloomberg` `clang-p2996` repo
```
git clone https://github.com/bloomberg/clang-p2996.git
cd clang-p2996
git checkout p2996
```
Create a build directory
```
mkdir build-p2996-apple
cd build-p2996-apple
```

Configure with CMake.  
Note: `CMAKE_INSTALL_PREFIX` controls where the toolchain is installed (used by all commands below);  
this guide uses `/opt/p2996/clang`.
```
cmake \
    -G Ninja \
    ../llvm \
    -DLLVM_TARGETS_TO_BUILD="AArch64" \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_ENABLE_RUNTIMES="libc;libunwind;libcxxabi;libcxx;compiler-rt" \
    -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/p2996/clang \
    -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
    -DLIBCXX_ENABLE_SHARED=ON \
    -DLIBCXX_ENABLE_STATIC=OFF \
    -DLIBCXX_ENABLE_FILESYSTEM=ON \
    -DLIBCXX_ENABLE_LOCALIZATION=ON
```

Build:
```
ninja
```

Install:
```
sudo ninja install
```

### How to check the Clang with Reflection version

```
/opt/p2996/clang/bin/clang++ --version
```
Expected output
```
clang version 21.0.0git (https://github.com/bloomberg/clang-p2996.git 4fd4f57ef9212c847ee05e034aed3bf7c59cb3cc)
Target: arm64-apple-darwin23.6.0
Thread model: posix
InstalledDir: /opt/p2996/clang/bin
```

### How to verify that Clang with Reflection builds viable executable

```
mkdir -p bin
/opt/p2996/clang/bin/clang++ \
    check_clang_libc++.cpp -o bin/check_clang_libc++ \
    -std=c++26 \
    -freflection-latest \
    -I/opt/p2996/clang/include/c++/v1 \
    -isysroot $(xcrun --sdk macosx --show-sdk-path) \
    -L/opt/p2996/clang/lib \
    -lc++abi \
    -Wl,-rpath,/opt/p2996/clang/lib \
    -DCLANG_BUILD_STRING="\"$(/opt/p2996/clang/bin/clang++ --version | tr '\n' ';')\""
bin/check_clang_libc++
```
Expected output
```
Custom Clang + libc++!
Built with: clang version 21.0.0git (https://github.com/bloomberg/clang-p2996.git 4fd4f57ef9212c847ee05e034aed3bf7c59cb3cc);Target: arm64-apple-darwin23.6.0;Thread model: posix;InstalledDir: /opt/p2996/clang/bin;
libc++ version = 210000
```

### How to run examples from the P2996 proposal using the Clang with Reflection

Examples implemented in `p2996_examples.cpp` (from P2996):

- 3.1 Back-And-Forth
- 3.2 Selecting Members
- 3.3 List of Types to List of Sizes
- 3.4 Implementing make_integer_sequence
- 3.5 Getting Class Layout
- 3.6 Enum to String
- 3.7 Parsing Command-Line Options
- 3.8 A Simple Tuple Type
- 3.9 A Simple Variant Type — omitted as the example depends on `boost/mp11.hpp`
- 3.10 Struct to Struct of Arrays
- 3.11 Parsing Command-Line Options II
- 3.12 A Universal Formatter
- 3.13 Implementing member-wise hash_append
- 3.14 Converting a Struct to a Tuple
- 3.15 Implementing tuple_cat (tuple concatenation)
- 3.16 Named Tuple
- 3.17 Compile-Time Ticket Counter
```
mkdir -p bin
/opt/p2996/clang/bin/clang++ \
    p2996_examples.cpp -o bin/p2996_examples \
    -std=c++26 \
    -freflection-latest \
    -I/opt/p2996/clang/include/c++/v1 \
    -isysroot $(xcrun --sdk macosx --show-sdk-path) \
    -L/opt/p2996/clang/lib \
    -lc++abi \
    -Wl,-rpath,/opt/p2996/clang/lib
bin/p2996_examples --file_name data.csv --count 3  --name John
```
Expected output
```
P2996 examples
3.1 Back-And-Forth
...
3.2 Selecting Members
3.3 List of Types to List of Sizes, reflection-based
...
3.3 List of Types to List of Sizes, template-based
...
3.4 Implementing make_integer_sequence
...
3.5 Getting Class Layout
...
3.6 Enum to String
...
3.7 Parsing Command-Line Options
...
3.8 A Simple Tuple Type
3.10 Struct to Struct of Arrays
...
3.11 Parsing Command-Line Options II
...
3.12 A Universal Formatter
...
3.13 Implementing member-wise hash_append
...
3.14 Converting a Struct to a Tuple
...
3.15 Implementing tuple_cat (tuple concatenation)
...
3.16 Named Tuple
...
3.17 Compile-Time Ticket Counter
...
```
