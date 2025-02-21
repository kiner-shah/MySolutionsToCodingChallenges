Current solution uses std::istream to read data from files/stdin.

Possible future optimizations:
- Using system-specific file system and using kernel hints like `posix_fadvise()` or [PrefetchVirtualMemory](https://stackoverflow.com/q/1201168/4688321).
- Using memory-mapped I/O.
- Using SIMD.
