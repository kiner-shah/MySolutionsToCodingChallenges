Implemented my own regex engine.

- Uses UTF-8 codepoints and the matcher processes one codepoint at a time.
- Haven't yet implemented matching for unicode category name.

### Grammar
I used the grammar from [here](https://github.com/kean/Regex/blob/master/grammar.ebnf).
The raidroad diagram for this grammar can be found [here](https://kean.blog/images/misc/grammar-diagram.xhtml).

This grammar is from the [article series](https://kean.blog/post/lets-build-regex) by Alex Grebenyuk aka kean.

I didn't follow the way he has implemented the engine. He used parser combinators which I found a bit hard to understand and implement. I implemented using simple recursive descent parser.

The only thing I followed from his articles was how to make the engine step by step:
- first implement the parser
- then implement the NFA (non-finite state automata)
- then implement the matcher

### Dependencies
- [doctest v2.4.12](https://github.com/doctest/doctest/releases/tag/v2.4.12).