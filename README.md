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
