# Clang with C++26 Reflection (P2996) — Prebuilt Images

This image provides a native toolchain with experimental C++26 Reflection support (WG21 P2996) via a Bloomberg-maintained Clang fork. It includes libc++/libc++abi/libunwind and compiler-rt runtimes and enables the reflection facilities with the `-freflection-latest` flag. These builds are intended for local experimentation only.

- Architectures: linux/arm64 and linux/amd64
- Install prefix inside image: `/opt/p2996/clang` (on `PATH`)
- Default C++ standard library: libc++

## Pull from Docker Hub

```
# arm64 (Apple Silicon, native arm64 Linux)
docker pull vsavkov/clang-p2996:arm64

# amd64 (Intel/AMD x86_64 Linux)
docker pull vsavkov/clang-p2996:amd64
```

## More Details

For build instructions, usage examples (including running proposal examples), and caveats, see the repository:

https://github.com/vsavkov/cplusplus-reflection-clang-p2996

> Note: This is an experimental toolchain; do not use for production artifacts.

