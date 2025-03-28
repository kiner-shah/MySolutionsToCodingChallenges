Implemented a compression tool as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-huffman).

The compression tool takes as input a text file containing multi-byte characters and generates a compressed output.

Since explicit handling of multi-byte characters is added, the tool only supports compression of text files. Binary files like executables doesn't work. For that to work, only single byte characters need to be handled which can simplify the logic and still work for multi-byte characters, although the compressed size can be different (more than current implementation).
