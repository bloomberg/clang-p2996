// check_clang_libc++.cpp
// To check that the clang and libc++ custom build works
// clang is installed in /opt/p2996/clang
// the directory /opt/p2996/clang is expected to have the following sub-directories
// bin	include	lib	libexec	share
/* To compile on macOS
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
*/

// To compile on Linux see README.md

// $ echo $(/opt/p2996/clang/bin/clang++ -E -dM - < /dev/null)

#include <iostream>

#ifndef CLANG_BUILD_STRING
#define CLANG_BUILD_STRING "unknown compiler"
#endif

int main() {
    std::cout << "Custom Clang + libc++!" << std::endl;
    std::cout << "Built with: " << CLANG_BUILD_STRING << "\n";

#ifdef _LIBCPP_VERSION
    std::cout << "libc++ version = " << _LIBCPP_VERSION << "\n";
#else
    std::cout << "Unknown libc++ version\n";
#endif

    return 0;
}
