Implemented word-count tool as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-wc).

Current solution uses std::istream to read data from files/stdin.
- word_count.cpp - uses std::mbrtowc() to convert multi-byte characters to wide characters. Using wide characters is not a good idea and is discouraged by the community.
- word_count_v2.cpp - uses a state-machine like [algorithm](https://writings.sh/post/en/utf8) to convert multi-byte characters to UTF-32 codepoints. Even better solution was suggested on [a reddit post](https://www.reddit.com/r/cpp_questions/comments/1jijknu/comment/mjswt16/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button) by a user named Dan13l_N.

Possible future optimizations:
- Using system-specific file system and using kernel hints like `posix_fadvise()` or [PrefetchVirtualMemory](https://stackoverflow.com/q/1201168/4688321).
- Using memory-mapped I/O.
- Using SIMD.
