Implemented a grep tool as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-grep).

### Test data
```
$ ls test.txt
test.txt

$ tree test_data
test_data
├── rockbands.txt
├── symbols.txt
├── test-subdir
│   └── BFS1985.txt
└── test.txt
```

### Note
Case insensitive search only implemented for ASCII characters. Implementing the same for UTF-8 characters will introduce a lot of complexity. I will need to use [CaseFolding.txt](https://www.unicode.org/Public/17.0.0/ucd/CaseFolding.txt) to get all mappings. Some of these map one codepoint to multiple code points (full case folding) which currently the matcher doesn't support.